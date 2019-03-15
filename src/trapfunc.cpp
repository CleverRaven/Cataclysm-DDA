#include "trap.h" // IWYU pragma: associated

#include "debug.h"
#include "event.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"

const mtype_id mon_blob( "mon_blob" );
const mtype_id mon_shadow( "mon_shadow" );
const mtype_id mon_shadow_snake( "mon_shadow_snake" );

const species_id ROBOT( "ROBOT" );

const skill_id skill_throw( "throw" );

const efftype_id effect_beartrap( "beartrap" );
const efftype_id effect_heavysnare( "heavysnare" );
const efftype_id effect_in_pit( "in_pit" );
const efftype_id effect_lightsnare( "lightsnare" );
const efftype_id effect_slimed( "slimed" );
const efftype_id effect_tetanus( "tetanus" );

static const trait_id trait_INFIMMUNE( "INFIMMUNE" );
static const trait_id trait_INFRESIST( "INFRESIST" );
static const trait_id trait_WINGS_BIRD( "WINGS_BIRD" );
static const trait_id trait_WINGS_BUTTERFLY( "WINGS_BUTTERFLY" );

// A pit becomes less effective as it fills with corpses.
float pit_effectiveness( const tripoint &p )
{
    units::volume corpse_volume = 0_ml;
    for( auto &pit_content : g->m.i_at( p ) ) {
        if( pit_content.is_corpse() ) {
            corpse_volume += pit_content.volume();
        }
    }

    // 10 zombies; see item::volume
    const units::volume filled_volume = 10 * units::from_milliliter<float>( 62500 );

    return std::max( 0.0f, 1.0f - corpse_volume / filled_volume );
}

void trapfunc::none( Creature *, const tripoint & )
{
}

void trapfunc::bubble( Creature *c, const tripoint &p )
{
    // tiny animals don't trigger bubble wrap
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_warning, _( "You step on some bubble wrap!" ),
                                  _( "<npcname> steps on some bubble wrap!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Stepped on bubble wrap." ),
                             pgettext( "memorial_female", "Stepped on bubble wrap." ) );
    }
    sounds::sound( p, 18, sounds::sound_t::alarm, _( "Pop!" ) );
    g->m.remove_trap( p );
}

void trapfunc::cot( Creature *c, const tripoint & )
{
    monster *z = dynamic_cast<monster *>( c );
    if( z != nullptr ) {
        // Haha, only monsters stumble over a cot, humans are smart.
        add_msg( m_good, _( "The %s stumbles over the cot" ), z->name().c_str() );
        c->moves -= 100;
    }
}

void trapfunc::beartrap( Creature *c, const tripoint &p )
{
    // tiny animals don't trigger bear traps
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    sounds::sound( p, 8, sounds::sound_t::combat, _( "SNAP!" ) );
    g->m.remove_trap( p );
    if( c != nullptr ) {
        // What got hit?
        const body_part hit = one_in( 2 ) ? bp_leg_l : bp_leg_r;

        // Messages
        c->add_memorial_log( pgettext( "memorial_male", "Caught by a beartrap." ),
                             pgettext( "memorial_female", "Caught by a beartrap." ) );
        c->add_msg_player_or_npc( m_bad, _( "A bear trap closes on your foot!" ),
                                  _( "A bear trap closes on <npcname>'s foot!" ) );

        // Actual effects
        c->add_effect( effect_beartrap, 1_turns, hit, true );
        damage_instance d;
        d.add_damage( DT_BASH, 12 );
        d.add_damage( DT_CUT, 18 );
        c->deal_damage( nullptr, hit, d );

        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            if( ( n->has_trait( trait_INFRESIST ) ) && ( one_in( 512 ) ) ) {
                n->add_effect( effect_tetanus, 1_turns, num_bp, true );
            } else if( ( !n->has_trait( trait_INFIMMUNE ) || !n->has_trait( trait_INFRESIST ) ) &&
                       ( one_in( 128 ) ) ) {
                n->add_effect( effect_tetanus, 1_turns, num_bp, true );
            }
        }
        c->check_dead_state();
    } else {
        g->m.spawn_item( p, "beartrap" );
    }
}

void trapfunc::board( Creature *c, const tripoint & )
{
    // tiny animals don't trigger spiked boards, they can squeeze between the nails
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        c->add_memorial_log( pgettext( "memorial_male", "Stepped on a spiked board." ),
                             pgettext( "memorial_female", "Stepped on a spiked board." ) );
        c->add_msg_player_or_npc( m_bad, _( "You step on a spiked board!" ),
                                  _( "<npcname> steps on a spiked board!" ) );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( z != nullptr ) {
            z->moves -= 80;
            z->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, rng( 3, 5 ) ) );
            z->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, rng( 3, 5 ) ) );
        } else {
            c->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, rng( 6, 10 ) ) );
            c->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, rng( 6, 10 ) ) );
            if( ( n->has_trait( trait_INFRESIST ) ) && ( one_in( 256 ) ) ) {
                n->add_effect( effect_tetanus, 1_turns, num_bp, true );
            } else if( ( !n->has_trait( trait_INFIMMUNE ) || !n->has_trait( trait_INFRESIST ) ) &&
                       ( one_in( 35 ) ) ) {
                n->add_effect( effect_tetanus, 1_turns, num_bp, true );
            }
        }
        c->check_dead_state();
    }
}

void trapfunc::caltrops( Creature *c, const tripoint & )
{
    // tiny animals don't trigger caltrops, they can squeeze between them
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        c->add_memorial_log( pgettext( "memorial_male", "Stepped on a caltrop." ),
                             pgettext( "memorial_female", "Stepped on a caltrop." ) );
        c->add_msg_player_or_npc( m_bad, _( "You step on a sharp metal caltrop!" ),
                                  _( "<npcname> steps on a sharp metal caltrop!" ) );
        monster *z = dynamic_cast<monster *>( c );
        if( z != nullptr ) {
            z->moves -= 80;
            c->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, rng( 9, 15 ) ) );
            c->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, rng( 9, 15 ) ) );
        } else {
            c->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, rng( 9, 30 ) ) );
            c->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, rng( 9, 30 ) ) );
        }
        c->check_dead_state();
    }
}

void trapfunc::tripwire( Creature *c, const tripoint &p )
{
    // tiny animals don't trigger tripwires, they just squeeze under it
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        c->add_memorial_log( pgettext( "memorial_male", "Tripped on a tripwire." ),
                             pgettext( "memorial_female", "Tripped on a tripwire." ) );
        c->add_msg_player_or_npc( m_bad, _( "You trip over a tripwire!" ),
                                  _( "<npcname> trips over a tripwire!" ) );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( z != nullptr ) {
            z->stumble();
            if( rng( 0, 10 ) > z->get_dodge() ) {
                z->deal_damage( nullptr, bp_torso, damage_instance( DT_TRUE, rng( 1, 4 ) ) );
            }
        } else if( n != nullptr ) {
            std::vector<tripoint> valid;
            for( const tripoint &jk : g->m.points_in_radius( p, 1 ) ) {
                if( g->is_empty( jk ) ) {
                    valid.push_back( jk );
                }
            }
            if( !valid.empty() ) {
                n->setpos( random_entry( valid ) );
            }
            n->moves -= 150;
            ///\EFFECT_DEX decreases chance of taking damage from a tripwire trap
            if( rng( 5, 20 ) > n->dex_cur ) {
                n->hurtall( rng( 1, 4 ), nullptr );
            }
            if( c == &g->u ) {
                g->update_map( g->u );
            }
        }
        c->check_dead_state();
    }
}

void trapfunc::crossbow( Creature *c, const tripoint &p )
{
    bool add_bolt = true;
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_neutral, _( "You trigger a crossbow trap!" ),
                                  _( "<npcname> triggers a crossbow trap!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a crossbow trap." ),
                             pgettext( "memorial_female", "Triggered a crossbow trap." ) );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            ///\EFFECT_DODGE reduces chance of being hit by crossbow trap
            if( !one_in( 4 ) && rng( 8, 20 ) > n->get_dodge() ) {
                body_part hit = num_bp;
                switch( rng( 1, 10 ) ) {
                    case  1:
                        if( one_in( 2 ) ) {
                            hit = bp_foot_l;
                        } else {
                            hit = bp_foot_r;
                        }
                        break;
                    case  2:
                    case  3:
                    case  4:
                        if( one_in( 2 ) ) {
                            hit = bp_leg_l;
                        } else {
                            hit = bp_leg_r;
                        }
                        break;
                    case  5:
                    case  6:
                    case  7:
                    case  8:
                    case  9:
                        hit = bp_torso;
                        break;
                    case 10:
                        hit = bp_head;
                        break;
                }
                //~ %s is bodypart
                n->add_msg_if_player( m_bad, _( "Your %s is hit!" ), body_part_name( hit ).c_str() );
                c->deal_damage( nullptr, hit, damage_instance( DT_CUT, rng( 20, 30 ) ) );
                add_bolt = !one_in( 10 );
            } else {
                n->add_msg_player_or_npc( m_neutral, _( "You dodge the shot!" ),
                                          _( "<npcname> dodges the shot!" ) );
            }
        } else if( z != nullptr ) {
            bool seen = g->u.sees( *z );
            int chance = 0;
            // adapted from shotgun code - chance of getting hit depends on size
            switch( z->type->size ) {
                case MS_TINY:
                    chance = 50;
                    break;
                case MS_SMALL:
                    chance =  8;
                    break;
                case MS_MEDIUM:
                    chance =  6;
                    break;
                case MS_LARGE:
                    chance =  4;
                    break;
                case MS_HUGE:
                    chance =  1;
                    break;
            }
            if( one_in( chance ) ) {
                if( seen ) {
                    add_msg( m_bad, _( "A bolt shoots out and hits the %s!" ), z->name().c_str() );
                }
                z->deal_damage( nullptr, bp_torso, damage_instance( DT_CUT, rng( 20, 30 ) ) );
                add_bolt = !one_in( 10 );
            } else if( seen ) {
                add_msg( m_neutral, _( "A bolt shoots out, but misses the %s." ), z->name().c_str() );
            }
        }
        c->check_dead_state();
    }
    g->m.remove_trap( p );
    g->m.spawn_item( p, "crossbow" );
    g->m.spawn_item( p, "string_36" );
    if( add_bolt ) {
        g->m.spawn_item( p, "bolt_steel", 1, 1 );
    }
}

void trapfunc::shotgun( Creature *c, const tripoint &p )
{
    sounds::sound( p, 60, sounds::sound_t::combat, _( "Kerblam!" ) );
    int shots = 1;
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_neutral, _( "You trigger a shotgun trap!" ),
                                  _( "<npcname> triggers a shotgun trap!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a shotgun trap." ),
                             pgettext( "memorial_female", "Triggered a shotgun trap." ) );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            ///\EFFECT_STR_MAX increases chance of two shots from shotgun trap
            shots = ( one_in( 8 ) || one_in( 20 - n->str_max ) ? 2 : 1 );
            if( g->m.tr_at( p ).loadid == tr_shotgun_1 ) {
                shots = 1;
            }
            ///\EFFECT_DODGE reduces chance of being hit by shotgun trap
            if( rng( 5, 50 ) > n->get_dodge() ) {
                body_part hit = num_bp;
                switch( rng( 1, 10 ) ) {
                    case  1:
                        if( one_in( 2 ) ) {
                            hit = bp_foot_l;
                        } else {
                            hit = bp_foot_r;
                        }
                        break;
                    case  2:
                    case  3:
                    case  4:
                        if( one_in( 2 ) ) {
                            hit = bp_leg_l;
                        } else {
                            hit = bp_leg_r;
                        }
                        break;
                    case  5:
                    case  6:
                    case  7:
                    case  8:
                    case  9:
                        hit = bp_torso;
                        break;
                    case 10:
                        hit = bp_head;
                        break;
                }
                //~ %s is bodypart
                n->add_msg_if_player( m_bad, _( "Your %s is hit!" ), body_part_name( hit ).c_str() );
                c->deal_damage( nullptr, hit, damage_instance( DT_CUT, rng( 40 * shots, 60 * shots ) ) );
            } else {
                n->add_msg_player_or_npc( m_neutral, _( "You dodge the shot!" ),
                                          _( "<npcname> dodges the shot!" ) );
            }
        } else if( z != nullptr ) {
            bool seen = g->u.sees( *z );
            int chance = 0;
            switch( z->type->size ) {
                case MS_TINY:
                    chance = 100;
                    break;
                case MS_SMALL:
                    chance =  16;
                    break;
                case MS_MEDIUM:
                    chance =  12;
                    break;
                case MS_LARGE:
                    chance =   8;
                    break;
                case MS_HUGE:
                    chance =   2;
                    break;
            }
            shots = ( one_in( 8 ) || one_in( chance ) ? 2 : 1 );
            if( g->m.tr_at( p ).loadid == tr_shotgun_1 ) {
                shots = 1;
            }
            if( seen ) {
                add_msg( m_bad, _( "A shotgun fires and hits the %s!" ), z->name().c_str() );
            }
            z->deal_damage( nullptr, bp_torso, damage_instance( DT_CUT, rng( 40 * shots, 60 * shots ) ) );
        }
        c->check_dead_state();
    }

    g->m.spawn_item( p, g->m.tr_at( p ).loadid == tr_shotgun_1 ? "shotgun_s" : "shotgun_d" );
    g->m.spawn_item( p, "string_36" );
    g->m.remove_trap( p );
}

void trapfunc::blade( Creature *c, const tripoint & )
{
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_bad, _( "A blade swings out and hacks your torso!" ),
                                  _( "A blade swings out and hacks <npcname>s torso!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a blade trap." ),
                             pgettext( "memorial_female", "Triggered a blade trap." ) );
        damage_instance d;
        d.add_damage( DT_BASH, 12 );
        d.add_damage( DT_CUT, 30 );
        c->deal_damage( nullptr, bp_torso, d );
        c->check_dead_state();
    }
}

void trapfunc::snare_light( Creature *c, const tripoint &p )
{
    sounds::sound( p, 2, sounds::sound_t::combat, _( "Snap!" ) );
    g->m.remove_trap( p );
    if( c != nullptr ) {
        // Determine what gets hit
        const body_part hit = one_in( 2 ) ? bp_leg_l : bp_leg_r;
        // Messages
        c->add_msg_player_or_npc( m_bad, _( "A snare closes on your leg." ),
                                  _( "A snare closes on <npcname>s leg." ) );
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a light snare." ),
                             pgettext( "memorial_female", "Triggered a light snare." ) );

        // Actual effects
        c->add_effect( effect_lightsnare, 1_turns, hit, true );
        monster *z = dynamic_cast<monster *>( c );
        if( z != nullptr && z->type->size == MS_TINY ) {
            z->deal_damage( nullptr, hit, damage_instance( DT_BASH, 10 ) );
        }
        c->check_dead_state();
    }
}

void trapfunc::snare_heavy( Creature *c, const tripoint &p )
{
    sounds::sound( p, 4, sounds::sound_t::combat, _( "Snap!" ) );
    g->m.remove_trap( p );
    if( c != nullptr ) {
        // Determine what got hit
        const body_part hit = one_in( 2 ) ? bp_leg_l : bp_leg_r;
        //~ %s is bodypart name in accusative.
        c->add_msg_player_or_npc( m_bad, _( "A snare closes on your %s." ),
                                  _( "A snare closes on <npcname>s %s." ), body_part_name_accusative( hit ).c_str() );
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a heavy snare." ),
                             pgettext( "memorial_female", "Triggered a heavy snare." ) );

        // Actual effects
        c->add_effect( effect_heavysnare, 1_turns, hit, true );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            damage_instance d;
            d.add_damage( DT_BASH, 10 );
            n->deal_damage( nullptr, hit, d );
        } else if( z != nullptr ) {
            int damage;
            switch( z->type->size ) {
                case MS_TINY:
                    damage = 20;
                    break;
                case MS_SMALL:
                    damage = 20;
                    break;
                case MS_MEDIUM:
                    damage = 10;
                    break;
                default:
                    damage = 0;
            }
            z->deal_damage( nullptr, hit, damage_instance( DT_BASH, damage ) );
        }
        c->check_dead_state();
    }
}

void trapfunc::landmine( Creature *c, const tripoint &p )
{
    // tiny animals are too light to trigger land mines
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_bad, _( "You trigger a land mine!" ),
                                  _( "<npcname> triggers a land mine!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Stepped on a land mine." ),
                             pgettext( "memorial_female", "Stepped on a land mine." ) );
    }
    g->explosion( p, 18, 0.5, false, 8 );
    g->m.remove_trap( p );
}

void trapfunc::boobytrap( Creature *c, const tripoint &p )
{
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_bad, _( "You trigger a booby trap!" ),
                                  _( "<npcname> triggers a booby trap!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a booby trap." ),
                             pgettext( "memorial_female", "Triggered a booby trap." ) );
    }
    g->explosion( p, 18, 0.6, false, 12 );
    g->m.remove_trap( p );
}

void trapfunc::telepad( Creature *c, const tripoint &p )
{
    //~ the sound of a telepad functioning
    sounds::sound( p, 6, sounds::sound_t::movement, _( "vvrrrRRMM*POP!*" ) );
    if( c != nullptr ) {
        monster *z = dynamic_cast<monster *>( c );
        // TODO: NPC don't teleport?
        if( c == &g->u ) {
            c->add_msg_if_player( m_warning, _( "The air shimmers around you..." ) );
            c->add_memorial_log( pgettext( "memorial_male", "Triggered a teleport trap." ),
                                 pgettext( "memorial_female", "Triggered a teleport trap." ) );
            g->teleport();
        } else if( z != nullptr ) {
            if( g->u.sees( *z ) ) {
                add_msg( _( "The air shimmers around the %s..." ), z->name().c_str() );
            }

            int tries = 0;
            int newposx = 0;
            int newposy = 0;
            do {
                newposx = rng( z->posx() - SEEX, z->posx() + SEEX );
                newposy = rng( z->posy() - SEEY, z->posy() + SEEY );
                tries++;
            } while( g->m.impassable( newposx, newposy ) && tries != 10 );

            if( tries == 10 ) {
                z->die_in_explosion( nullptr );
            } else if( monster *const mon_hit = g->critter_at<monster>( {newposx, newposy, z->posz()} ) ) {
                if( g->u.sees( *z ) ) {
                    add_msg( m_good, _( "The %1$s teleports into a %2$s, killing them both!" ),
                             z->name().c_str(), mon_hit->name().c_str() );
                }
                mon_hit->die_in_explosion( z );
            } else {
                z->setpos( {newposx, newposy, z->posz()} );
            }
        }
    }
}

void trapfunc::goo( Creature *c, const tripoint &p )
{
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_bad, _( "You step in a puddle of thick goo." ),
                                  _( "<npcname> steps in a puddle of thick goo." ) );
        c->add_memorial_log( pgettext( "memorial_male", "Stepped into thick goo." ),
                             pgettext( "memorial_female", "Stepped into thick goo." ) );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            n->add_env_effect( effect_slimed, bp_foot_l, 6, 2_minutes );
            n->add_env_effect( effect_slimed, bp_foot_r, 6, 2_minutes );
            if( one_in( 3 ) ) {
                n->add_msg_if_player( m_bad, _( "The acidic goo eats away at your feet." ) );
                n->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 5 ) );
                n->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, 5 ) );
                n->check_dead_state();
            }
        } else if( z != nullptr ) {
            //All monsters except for blobs get a speed decrease
            if( z->type->id != mon_blob ) {
                z->set_speed_base( z->get_speed_base() - 15 );
                //All monsters that aren't blobs or robots transform into a blob
                if( !z->type->in_species( ROBOT ) ) {
                    z->poly( mon_blob );
                    z->set_hp( z->get_speed() );
                }
            } else {
                z->set_speed_base( z->get_speed_base() + 15 );
                z->set_hp( z->get_speed() );
            }
        }
    }
    g->m.remove_trap( p );
}

void trapfunc::dissector( Creature *c, const tripoint &p )
{
    if( c == nullptr ) {
        return;
    }

    monster *z = dynamic_cast<monster *>( c );
    if( z != nullptr && z->type->in_species( ROBOT ) ) {
        //The monster is a robot. So the dissector should not try to dissect the monsters flesh.
        sounds::sound( p, 4, sounds::sound_t::speech,
                       _( "BEEPBOOP! Please remove non-organic object." ) ); //Dissector error sound.
        c->add_msg_player_or_npc( m_bad, _( "The dissector lights up, and shuts down." ),
                                  _( "The dissector lights up, and shuts down." ) );
        return;
    }

    //~ the sound of a dissector dissecting
    sounds::sound( p, 10, sounds::sound_t::combat, _( "BRZZZAP!" ) );
    c->add_msg_player_or_npc( m_bad, _( "Electrical beams emit from the floor and slice your flesh!" ),
                              _( "Electrical beams emit from the floor and slice <npcname>s flesh!" ) );
    c->add_memorial_log( pgettext( "memorial_male", "Stepped into a dissector." ),
                         pgettext( "memorial_female", "Stepped into a dissector." ) );
    c->deal_damage( nullptr, bp_head, damage_instance( DT_CUT, 15 ) );
    c->deal_damage( nullptr, bp_torso, damage_instance( DT_CUT, 20 ) );
    c->deal_damage( nullptr, bp_arm_r, damage_instance( DT_CUT, 12 ) );
    c->deal_damage( nullptr, bp_arm_l, damage_instance( DT_CUT, 12 ) );
    c->deal_damage( nullptr, bp_hand_r, damage_instance( DT_CUT, 10 ) );
    c->deal_damage( nullptr, bp_hand_l, damage_instance( DT_CUT, 10 ) );
    c->deal_damage( nullptr, bp_leg_r, damage_instance( DT_CUT, 12 ) );
    c->deal_damage( nullptr, bp_leg_r, damage_instance( DT_CUT, 12 ) );
    c->deal_damage( nullptr, bp_foot_l, damage_instance( DT_CUT, 10 ) );
    c->deal_damage( nullptr, bp_foot_r, damage_instance( DT_CUT, 10 ) );
    c->check_dead_state();
}

void trapfunc::pit( Creature *c, const tripoint &p )
{
    // tiny animals aren't hurt by falling into pits
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        const float eff = pit_effectiveness( p );
        c->add_msg_player_or_npc( m_bad, _( "You fall in a pit!" ), _( "<npcname> falls in a pit!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Fell in a pit." ),
                             pgettext( "memorial_female", "Fell in a pit." ) );
        c->add_effect( effect_in_pit, 1_turns, num_bp, true );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            if( ( n->has_trait( trait_WINGS_BIRD ) ) || ( ( one_in( 2 ) ) &&
                    ( n->has_trait( trait_WINGS_BUTTERFLY ) ) ) ) {
                n->add_msg_if_player( _( "You flap your wings and flutter down gracefully." ) );
            } else if( n->has_active_bionic( bionic_id( "bio_shock_absorber" ) ) ) {
                n->add_msg_if_player( m_info,
                                      _( "You hit the ground hard, but your shock absorbers handle the impact admirably!" ) );
            } else {
                int dodge = n->get_dodge();
                ///\EFFECT_DODGE reduces damage taken falling into a pit
                int damage = eff * rng( 10, 20 ) - rng( dodge, dodge * 5 );
                if( damage > 0 ) {
                    n->add_msg_if_player( m_bad, _( "You hurt yourself!" ) );
                    n->hurtall( rng( int( damage / 2 ), damage ), n ); // like the message says \-:
                    n->deal_damage( nullptr, bp_leg_l, damage_instance( DT_BASH, damage ) );
                    n->deal_damage( nullptr, bp_leg_r, damage_instance( DT_BASH, damage ) );
                } else {
                    n->add_msg_if_player( _( "You land nimbly." ) );
                }
            }
        } else if( z != nullptr ) {
            z->deal_damage( nullptr, bp_leg_l, damage_instance( DT_BASH, eff * rng( 10, 20 ) ) );
            z->deal_damage( nullptr, bp_leg_r, damage_instance( DT_BASH, eff * rng( 10, 20 ) ) );
        }
        c->check_dead_state();
    }
}

void trapfunc::pit_spikes( Creature *c, const tripoint &p )
{
    // tiny animals aren't hurt by falling into spiked pits
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_bad, _( "You fall in a spiked pit!" ),
                                  _( "<npcname> falls in a spiked pit!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Fell into a spiked pit." ),
                             pgettext( "memorial_female", "Fell into a spiked pit." ) );
        c->add_effect( effect_in_pit, 1_turns, num_bp, true );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            int dodge = n->get_dodge();
            int damage = pit_effectiveness( p ) * rng( 20, 50 );
            if( ( n->has_trait( trait_WINGS_BIRD ) ) || ( ( one_in( 2 ) ) &&
                    ( n->has_trait( trait_WINGS_BUTTERFLY ) ) ) ) {
                n->add_msg_if_player( _( "You flap your wings and flutter down gracefully." ) );
            } else if( n->has_active_bionic( bionic_id( "bio_shock_absorber" ) ) ) {
                n->add_msg_if_player( m_info,
                                      _( "You hit the ground hard, but your shock absorbers handle the impact admirably!" ) );
                ///\EFFECT_DODGE reduces chance of landing on spikes in spiked pit
            } else if( 0 == damage || rng( 5, 30 ) < dodge ) {
                n->add_msg_if_player( _( "You avoid the spikes within." ) );
            } else {
                body_part hit = num_bp;
                switch( rng( 1, 10 ) ) {
                    case  1:
                        hit = bp_leg_l;
                        break;
                    case  2:
                        hit = bp_leg_r;
                        break;
                    case  3:
                        hit = bp_arm_l;
                        break;
                    case  4:
                        hit = bp_arm_r;
                        break;
                    case  5:
                    case  6:
                    case  7:
                    case  8:
                    case  9:
                    case 10:
                        hit = bp_torso;
                        break;
                }
                n->add_msg_if_player( m_bad, _( "The spikes impale your %s!" ),
                                      body_part_name_accusative( hit ).c_str() );
                n->deal_damage( nullptr, hit, damage_instance( DT_CUT, damage ) );
                if( ( n->has_trait( trait_INFRESIST ) ) && ( one_in( 256 ) ) ) {
                    n->add_effect( effect_tetanus, 1_turns, num_bp, true );
                } else if( ( !n->has_trait( trait_INFIMMUNE ) || !n->has_trait( trait_INFRESIST ) ) &&
                           ( one_in( 35 ) ) ) {
                    n->add_effect( effect_tetanus, 1_turns, num_bp, true );
                }
            }
        } else if( z != nullptr ) {
            z->deal_damage( nullptr, bp_torso, damage_instance( DT_CUT, rng( 20, 50 ) ) );
        }
        c->check_dead_state();
    }
    if( one_in( 4 ) ) {
        if( g->u.sees( p ) ) {
            add_msg( _( "The spears break!" ) );
        }
        g->m.ter_set( p, t_pit );
        for( int i = 0; i < 4; i++ ) { // 4 spears to a pit
            if( one_in( 3 ) ) {
                g->m.spawn_item( p, "pointy_stick" );
            }
        }
    }
}

void trapfunc::pit_glass( Creature *c, const tripoint &p )
{
    // tiny animals aren't hurt by falling into glass pits
    if( c != nullptr && c->get_size() == MS_TINY ) {
        return;
    }
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_bad, _( "You fall in a pit filled with glass shards!" ),
                                  _( "<npcname> falls in pit filled with glass shards!" ) );
        c->add_memorial_log( pgettext( "memorial_male", "Fell into a pit filled with glass shards." ),
                             pgettext( "memorial_female", "Fell into a pit filled with glass shards." ) );
        c->add_effect( effect_in_pit, 1_turns, num_bp, true );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            int dodge = n->get_dodge();
            int damage = pit_effectiveness( p ) * rng( 15, 35 );
            if( ( n->has_trait( trait_WINGS_BIRD ) ) || ( ( one_in( 2 ) ) &&
                    ( n->has_trait( trait_WINGS_BUTTERFLY ) ) ) ) {
                n->add_msg_if_player( _( "You flap your wings and flutter down gracefully." ) );
            } else if( n->has_active_bionic( bionic_id( "bio_shock_absorber" ) ) ) {
                n->add_msg_if_player( m_info,
                                      _( "You hit the ground hard, but your shock absorbers handle the impact admirably!" ) );
                ///\EFFECT_DODGE reduces chance of landing on glass in glass pit
            } else if( 0 == damage || rng( 5, 30 ) < dodge ) {
                n->add_msg_if_player( _( "You avoid the glass shards within." ) );
            } else {
                body_part hit = num_bp;
                switch( rng( 1, 10 ) ) {
                    case  1:
                        hit = bp_leg_l;
                        break;
                    case  2:
                        hit = bp_leg_r;
                        break;
                    case  3:
                        hit = bp_arm_l;
                        break;
                    case  4:
                        hit = bp_arm_r;
                        break;
                    case  5:
                        hit = bp_foot_l;
                        break;
                    case  6:
                        hit = bp_foot_r;
                        break;
                    case  7:
                    case  8:
                    case  9:
                    case 10:
                        hit = bp_torso;
                        break;
                }
                n->add_msg_if_player( m_bad, _( "The glass shards slash your %s!" ),
                                      body_part_name_accusative( hit ).c_str() );
                n->deal_damage( nullptr, hit, damage_instance( DT_CUT, damage ) );
                if( ( n->has_trait( trait_INFRESIST ) ) && ( one_in( 256 ) ) ) {
                    n->add_effect( effect_tetanus, 1_turns, num_bp, true );
                } else if( ( !n->has_trait( trait_INFIMMUNE ) || !n->has_trait( trait_INFRESIST ) ) &&
                           ( one_in( 35 ) ) ) {
                    n->add_effect( effect_tetanus, 1_turns, num_bp, true );
                }
            }
        } else if( z != nullptr ) {
            z->deal_damage( nullptr, bp_torso, damage_instance( DT_CUT, rng( 20, 50 ) ) );
        }
        c->check_dead_state();
    }
    if( one_in( 5 ) ) {
        if( g->u.sees( p ) ) {
            add_msg( _( "The shards shatter!" ) );
        }
        g->m.ter_set( p, t_pit );
        for( int i = 0; i < 20; i++ ) { // 20 shards in a pit.
            if( one_in( 3 ) ) {
                g->m.spawn_item( p, "glass_shard" );
            }
        }
    }
}

void trapfunc::lava( Creature *c, const tripoint &p )
{
    if( c != nullptr ) {
        c->add_msg_player_or_npc( m_bad, _( "The %s burns you horribly!" ), _( "The %s burns <npcname>!" ),
                                  g->m.tername( p ).c_str() );
        c->add_memorial_log( pgettext( "memorial_male", "Stepped into lava." ),
                             pgettext( "memorial_female", "Stepped into lava." ) );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            n->deal_damage( nullptr, bp_foot_l, damage_instance( DT_HEAT, 20 ) );
            n->deal_damage( nullptr, bp_foot_r, damage_instance( DT_HEAT, 20 ) );
            n->deal_damage( nullptr, bp_leg_l, damage_instance( DT_HEAT, 20 ) );
            n->deal_damage( nullptr, bp_leg_r, damage_instance( DT_HEAT, 20 ) );
        } else if( z != nullptr ) {
            // MATERIALS-TODO: use fire resistance
            int dam = 30;
            if( z->made_of_any( Creature::cmat_flesh ) ) {
                dam = 50;
            }
            if( z->made_of( material_id( "veggy" ) ) ) {
                dam = 80;
            }
            if( z->made_of( LIQUID ) || z->made_of_any( Creature::cmat_flammable ) ) {
                dam = 200;
            }
            if( z->made_of( material_id( "stone" ) ) ) {
                dam = 15;
            }
            if( z->made_of( material_id( "kevlar" ) ) || z->made_of( material_id( "steel" ) ) ) {
                dam = 5;
            }
            z->deal_damage( nullptr, bp_torso, damage_instance( DT_HEAT, dam ) );
        }
        c->check_dead_state();
    }
}

// STUB
void trapfunc::portal( Creature *c, const tripoint &p )
{
    // TODO: make this do something unique and interesting
    telepad( c, p );
}

// Don't ask NPCs - they always want to do the first thing that comes to their minds
bool query_for_item( const player *pl, const std::string &itemname, const char *que )
{
    return pl->has_amount( itemname, 1 ) && ( !pl->is_player() || query_yn( que ) );
}

static tripoint random_neighbor( tripoint center )
{
    center.x += rng( -1, 1 );
    center.y += rng( -1, 1 );
    return center;
}

static bool sinkhole_safety_roll( player *p, const std::string &itemname, const int diff )
{
    ///\EFFECT_STR increases chance to attach grapnel, bullwhip, or rope when falling into a sinkhole

    ///\EFFECT_DEX increases chance to attach grapnel, bullwhip, or rope when falling into a sinkhole

    ///\EFFECT_THROW increases chance to attach grapnel, bullwhip, or rope when falling into a sinkhole
    const int roll = rng( p->get_skill_level( skill_throw ),
                          p->get_skill_level( skill_throw ) + p->str_cur + p->dex_cur );
    if( roll < diff ) {
        p->add_msg_if_player( m_bad, _( "You fail to attach it..." ) );
        p->use_amount( itemname, 1 );
        g->m.spawn_item( random_neighbor( p->pos() ), itemname );
        return false;
    }

    std::vector<tripoint> safe;
    for( const tripoint &tmp : g->m.points_in_radius( p->pos(), 1 ) ) {
        if( g->m.passable( tmp ) && g->m.tr_at( tmp ).loadid != tr_pit ) {
            safe.push_back( tmp );
        }
    }
    if( safe.empty() ) {
        p->add_msg_if_player( m_bad, _( "There's nowhere to pull yourself to, and you sink!" ) );
        p->use_amount( itemname, 1 );
        g->m.spawn_item( random_neighbor( p->pos() ), itemname );
        return false;
    } else {
        p->add_msg_player_or_npc( m_good, _( "You pull yourself to safety!" ),
                                  _( "<npcname> steps on a sinkhole, but manages to pull themselves to safety." ) );
        p->setpos( random_entry( safe ) );
        if( p == &g->u ) {
            g->update_map( *p );
        }

        return true;
    }
}

void trapfunc::sinkhole( Creature *c, const tripoint &p )
{
    player *pl = dynamic_cast<player *>( c );
    if( pl == nullptr ) {
        // TODO: Handle monsters
        return;
    }

    pl->add_memorial_log( pgettext( "memorial_male", "Stepped into a sinkhole." ),
                          pgettext( "memorial_female", "Stepped into a sinkhole." ) );
    bool success = false;
    if( query_for_item( pl, "grapnel",
                        _( "You step into a sinkhole!  Throw your grappling hook out to try to catch something?" ) ) ) {
        success = sinkhole_safety_roll( pl, "grapnel", 6 );
    } else if( query_for_item( pl, "bullwhip",
                               _( "You step into a sinkhole!  Throw your whip out to try and snag something?" ) ) ) {
        success = sinkhole_safety_roll( pl, "bullwhip", 8 );
    } else if( query_for_item( pl, "rope_30",
                               _( "You step into a sinkhole!  Throw your rope out to try to catch something?" ) ) ) {
        success = sinkhole_safety_roll( pl, "rope_30", 12 );
    }

    pl->add_msg_player_or_npc( m_warning, _( "The sinkhole collapses!" ),
                               _( "A sinkhole under <npcname> collapses!" ) );
    g->m.remove_trap( p );
    g->m.ter_set( p, t_pit );
    if( success ) {
        return;
    }

    pl->moves -= 100;
    pl->add_msg_player_or_npc( m_bad, _( "You fall into the sinkhole!" ),
                               _( "<npcname> falls into a sinkhole!" ) );
    pit( c, p );
}

void trapfunc::ledge( Creature *c, const tripoint &p )
{
    if( c == nullptr ) {
        return;
    }

    monster *m = dynamic_cast<monster *>( c );
    if( m != nullptr && m->has_flag( MF_FLIES ) ) {
        return;
    }

    if( !g->m.has_zlevels() ) {
        if( c == &g->u ) {
            add_msg( m_warning, _( "You fall down a level!" ) );
            g->u.add_memorial_log( pgettext( "memorial_male", "Fell down a ledge." ),
                                   pgettext( "memorial_female", "Fell down a ledge." ) );
            g->vertical_move( -1, true );
            if( g->u.has_trait( trait_WINGS_BIRD ) || ( one_in( 2 ) &&
                    g->u.has_trait( trait_WINGS_BUTTERFLY ) ) ) {
                add_msg( _( "You flap your wings and flutter down gracefully." ) );
            } else if( g->u.has_active_bionic( bionic_id( "bio_shock_absorber" ) ) ) {
                add_msg( m_info,
                         _( "You hit the ground hard, but your shock absorbers handle the impact admirably!" ) );
            } else {
                g->u.impact( 20, p );
            }
        } else {
            c->add_msg_if_npc( _( "<npcname> falls down a level!" ) );
            tripoint dest = c->pos();
            dest.z--;
            c->impact( 20, dest );
            if( g->m.has_flag( TFLAG_NO_FLOOR, dest ) && m != nullptr ) {
                // don't kill until they hit the ground so that subsequent ledges will trigger
                m->set_hp( std::max( m->get_hp(), 1 ) );
            }

            c->setpos( dest );
            if( m != nullptr ) {
                g->remove_zombie( *m );
                overmap_buffer.despawn_monster( *m );
            }
        }

        return;
    }

    int height = 0;
    tripoint where = p;
    tripoint below = where;
    below.z--;
    while( g->m.valid_move( where, below, false, true ) ) {
        where.z--;
        if( g->critter_at( where ) != nullptr ) {
            where.z++;
            break;
        }

        below.z--;
        height++;
    }

    if( height == 0 && c->is_player() ) {
        // For now just special case player, NPCs don't "zedwalk"
        Creature *critter = g->critter_at( below, true );
        if( critter == nullptr || !critter->is_monster() ) {
            return;
        }

        std::vector<tripoint> valid;
        for( const tripoint &pt : g->m.points_in_radius( below, 1 ) ) {
            if( g->is_empty( pt ) ) {
                valid.push_back( pt );
            }
        }

        if( valid.empty() ) {
            critter->setpos( c->pos() );
            add_msg( m_bad, _( "You fall down under %s!" ), critter->disp_name().c_str() );
        } else {
            critter->setpos( random_entry( valid ) );
        }

        height++;
        where.z--;
    } else if( height == 0 ) {
        return;
    }

    c->add_msg_if_npc( _( "<npcname> falls down a level!" ) );
    player *pl = dynamic_cast<player *>( c );
    if( pl == nullptr ) {
        c->setpos( where );
        c->impact( height * 10, where );
        return;
    }

    if( pl->is_player() ) {
        add_msg( m_warning, _( "You fall down a level!" ) );
        g->u.add_memorial_log( pgettext( "memorial_male", "Fell down a ledge." ),
                               pgettext( "memorial_female", "Fell down a ledge." ) );
        g->vertical_move( -height, true );
    } else {
        pl->setpos( where );
    }
    if( pl->has_trait( trait_WINGS_BIRD ) || ( one_in( 2 ) &&
            pl->has_trait( trait_WINGS_BUTTERFLY ) ) ) {
        pl->add_msg_player_or_npc( _( "You flap your wings and flutter down gracefully." ),
                                   _( "<npcname> flaps their wings and flutters down gracefully." ) );
    } else if( pl->has_active_bionic( bionic_id( "bio_shock_absorber" ) ) ) {
        pl->add_msg_if_player( m_info,
                               _( "You hit the ground hard, but your shock absorbers handle the impact admirably!" ) );
    } else {
        pl->impact( height * 10, where );
    }
}

void trapfunc::temple_flood( Creature *c, const tripoint &p )
{
    // Monsters and npcs are completely ignored here, should they?
    if( c == &g->u ) {
        add_msg( m_warning, _( "You step on a loose tile, and water starts to flood the room!" ) );
        g->u.add_memorial_log( pgettext( "memorial_male", "Triggered a flood trap." ),
                               pgettext( "memorial_female", "Triggered a flood trap." ) );
        tripoint tmp = p;
        int &i = tmp.x;
        int &j = tmp.y;
        for( i = 0; i < MAPSIZE_X; i++ ) {
            for( j = 0; j < MAPSIZE_Y; j++ ) {
                if( g->m.tr_at( tmp ).loadid == tr_temple_flood ) {
                    g->m.remove_trap( tmp );
                }
            }
        }
        g->events.add( EVENT_TEMPLE_FLOOD, calendar::turn + 3_turns );
    }
}

void trapfunc::temple_toggle( Creature *c, const tripoint &p )
{
    // Monsters and npcs are completely ignored here, should they?
    if( c == &g->u ) {
        add_msg( _( "You hear the grinding of shifting rock." ) );
        const ter_id type = g->m.ter( p );
        tripoint tmp = p;
        int &i = tmp.x;
        int &j = tmp.y;
        for( i = 0; i < MAPSIZE_X; i++ ) {
            for( j = 0; j < MAPSIZE_Y; j++ ) {
                if( type == t_floor_red ) {
                    if( g->m.ter( tmp ) == t_rock_green ) {
                        g->m.ter_set( tmp, t_floor_green );
                    } else if( g->m.ter( tmp ) == t_floor_green ) {
                        g->m.ter_set( tmp, t_rock_green );
                    }
                } else if( type == t_floor_green ) {
                    if( g->m.ter( tmp ) == t_rock_blue ) {
                        g->m.ter_set( tmp, t_floor_blue );
                    } else if( g->m.ter( tmp ) == t_floor_blue ) {
                        g->m.ter_set( tmp, t_rock_blue );
                    }
                } else if( type == t_floor_blue ) {
                    if( g->m.ter( tmp ) == t_rock_red ) {
                        g->m.ter_set( tmp, t_floor_red );
                    } else if( g->m.ter( tmp ) == t_floor_red ) {
                        g->m.ter_set( tmp, t_rock_red );
                    }
                }
            }
        }
    }
}

void trapfunc::glow( Creature *c, const tripoint &p )
{
    if( c != nullptr ) {
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            if( one_in( 3 ) ) {
                n->add_msg_if_player( m_bad, _( "You're bathed in radiation!" ) );
                n->irradiate( rng( 10, 30 ) );
            } else if( one_in( 4 ) ) {
                n->add_msg_if_player( m_bad, _( "A blinding flash strikes you!" ) );
                g->flashbang( p );
            } else {
                c->add_msg_if_player( _( "Small flashes surround you." ) );
            }
        } else if( z != nullptr && one_in( 3 ) ) {
            z->deal_damage( nullptr, bp_torso, damage_instance( DT_ACID, rng( 5, 10 ) ) );
            z->set_speed_base( z->get_speed_base() * 0.9 );
        }
        c->check_dead_state();
    }
}

void trapfunc::hum( Creature * /*c*/, const tripoint &p )
{
    int volume = rng( 1, 200 );
    std::string sfx;
    if( volume <= 10 ) {
        //~ a quiet humming sound
        sfx = _( "hrm" );
    } else if( volume <= 50 ) {
        //~ a humming sound
        sfx = _( "hrmmm" );
    } else if( volume <= 100 ) {
        //~ a loud humming sound
        sfx = _( "HRMMM" );
    } else {
        //~ a very loud humming sound
        sfx = _( "VRMMMMMM" );
    }
    sounds::sound( p, volume, sounds::sound_t::activity, sfx );
}

void trapfunc::shadow( Creature *c, const tripoint &p )
{
    if( c != &g->u ) {
        return;
    }
    // Monsters and npcs are completely ignored here, should they?
    g->u.add_memorial_log( pgettext( "memorial_male", "Triggered a shadow trap." ),
                           pgettext( "memorial_female", "Triggered a shadow trap." ) );
    int tries = 0;
    tripoint monp = p;
    do {
        if( one_in( 2 ) ) {
            monp.x = rng( g->u.posx() - 5, g->u.posx() + 5 );
            monp.y = ( one_in( 2 ) ? g->u.posy() - 5 : g->u.posy() + 5 );
        } else {
            monp.x = ( one_in( 2 ) ? g->u.posx() - 5 : g->u.posx() + 5 );
            monp.y = rng( g->u.posy() - 5, g->u.posy() + 5 );
        }
    } while( tries < 5 && !g->is_empty( monp ) &&
             !g->m.sees( monp, g->u.pos(), 10 ) );

    if( tries < 5 ) { // @todo: tries increment is missing, so this expression is always true
        if( monster *const spawned = g->summon_mon( mon_shadow, monp ) ) {
            add_msg( m_warning, _( "A shadow forms nearby." ) );
            spawned->reset_special_rng( "DISAPPEAR" );
        }
        g->m.remove_trap( p );
    }
}

void trapfunc::drain( Creature *c, const tripoint & )
{
    if( c != nullptr ) {
        c->add_msg_if_player( m_bad, _( "You feel your life force sapping away." ) );
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a life-draining trap." ),
                             pgettext( "memorial_female", "Triggered a life-draining trap." ) );
        monster *z = dynamic_cast<monster *>( c );
        player *n = dynamic_cast<player *>( c );
        if( n != nullptr ) {
            n->hurtall( 1, nullptr );
        } else if( z != nullptr ) {
            z->deal_damage( nullptr, bp_torso, damage_instance( DT_TRUE, 1 ) );
        }
        c->check_dead_state();
    }
}

void trapfunc::snake( Creature *c, const tripoint &p )
{
    //~ the sound a snake makes
    sounds::sound( p, 10, sounds::sound_t::movement, _( "ssssssss" ) );
    if( one_in( 6 ) ) {
        g->m.remove_trap( p );
    }
    if( c != nullptr ) {
        c->add_memorial_log( pgettext( "memorial_male", "Triggered a shadow snake trap." ),
                             pgettext( "memorial_female", "Triggered a shadow snake trap." ) );
    }
    if( one_in( 3 ) ) {
        int tries = 0;
        tripoint monp = p;
        // This spawns snakes only when the player can see them, why?
        do {
            tries++;
            if( one_in( 2 ) ) {
                monp.x = rng( g->u.posx() - 5, g->u.posx() + 5 );
                monp.y = ( one_in( 2 ) ? g->u.posy() - 5 : g->u.posy() + 5 );
            } else {
                monp.x = ( one_in( 2 ) ? g->u.posx() - 5 : g->u.posx() + 5 );
                monp.y = rng( g->u.posy() - 5, g->u.posy() + 5 );
            }
        } while( tries < 5 && !g->is_empty( monp ) &&
                 !g->m.sees( monp, g->u.pos(), 10 ) );

        if( tries < 5 ) { // @todo: tries increment is missing, so this expression is always true
            if( monster *const spawned = g->summon_mon( mon_shadow_snake, p ) ) {
                add_msg( m_warning, _( "A shadowy snake forms nearby." ) );
                spawned->reset_special_rng( "DISAPPEAR" );
                g->m.remove_trap( p );
            }
        }
    }
}

/**
 * Takes the name of a trap function and returns a function pointer to it.
 * @param function_name The name of the trapfunc function to find.
 * @return A function object with a pointer to the matched function,
 *         or to trapfunc::none if there is no match.
 */
const trap_function &trap_function_from_string( const std::string &function_name )
{
    static const std::unordered_map<std::string, trap_function> funmap = {{
            { "none", trapfunc::none },
            { "bubble", trapfunc::bubble },
            { "cot", trapfunc::cot },
            { "beartrap", trapfunc::beartrap },
            { "board", trapfunc::board },
            { "caltrops", trapfunc::caltrops },
            { "tripwire", trapfunc::tripwire },
            { "crossbow", trapfunc::crossbow },
            { "shotgun", trapfunc::shotgun },
            { "blade", trapfunc::blade },
            { "snare_light", trapfunc::snare_light },
            { "snare_heavy", trapfunc::snare_heavy },
            { "landmine", trapfunc::landmine },
            { "telepad", trapfunc::telepad },
            { "goo", trapfunc::goo },
            { "dissector", trapfunc::dissector },
            { "sinkhole", trapfunc::sinkhole },
            { "pit", trapfunc::pit },
            { "pit_spikes", trapfunc::pit_spikes },
            { "pit_glass", trapfunc::pit_glass },
            { "lava", trapfunc::lava },
            { "portal", trapfunc::portal },
            { "ledge", trapfunc::ledge },
            { "boobytrap", trapfunc::boobytrap },
            { "temple_flood", trapfunc::temple_flood },
            { "temple_toggle", trapfunc::temple_toggle },
            { "glow", trapfunc::glow },
            { "hum", trapfunc::hum },
            { "shadow", trapfunc::shadow },
            { "drain", trapfunc::drain },
            { "snake", trapfunc::snake }
        }
    };

    const auto iter = funmap.find( function_name );
    if( iter != funmap.end() ) {
        return iter->second;
    }

    debugmsg( "Could not find a trapfunc function matching '%s'!", function_name.c_str() );
    static const trap_function null_fun = trapfunc::none;
    return null_fun;
}
