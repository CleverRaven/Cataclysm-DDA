#include "mondefense.h"
#include "monster.h"
#include "creature.h"
#include "damage.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "messages.h"
#include "map.h"
#include "translations.h"
#include "field.h"

void mdefense::none( monster &, Creature *, const dealt_projectile_attack * )
{
}

void mdefense::zapback( monster &m, Creature *const source,
                        dealt_projectile_attack const *const proj )
{
    // Not a melee attack, attacker lucked out or out of range
    if( source == nullptr || proj != nullptr ||
        rng( 0, 100 ) > m.def_chance || rl_dist( m.pos(), source->pos() ) > 1 ) {
        return;
    }

    if( source->is_elec_immune() ) {
        return;
    }

    // Players/NPCs can avoid the shock by using non-conductive weapons
    player const *const foe = dynamic_cast<player *>( source );
    if( foe != nullptr && !foe->weapon.conductive() && !foe->unarmed_attack() ) {
        return;
    }

    damage_instance const shock {
        DT_ELECTRIC, static_cast<float>( rng( 1, 5 ) )
    };
    source->deal_damage( &m, bp_arm_l, shock );
    source->deal_damage( &m, bp_arm_r, shock );

    if( g->u.sees( source->pos() ) ) {
        auto const msg_type = ( source == &g->u ) ? m_bad : m_info;
        add_msg( msg_type, _( "Striking the %1$s shocks %2$s!" ),
                 m.name().c_str(), source->disp_name().c_str() );
    }
    source->check_dead_state();
}

static int sign( int arg )
{
    if( arg > 0 ) {
        return 1;
    }

    if( arg < 0 ) {
        return -1;
    }

    return 0;
}

void mdefense::acidsplash( monster &m, Creature *const source,
                           dealt_projectile_attack const *const proj )
{
    // Would be useful to have the attack data here, for cutting vs. bashing etc.
    if( proj != nullptr && proj->dealt_dam.total_damage() <= 0 ) {
        // Projectile didn't penetrate the target, no acid will splash out of it.
        return;
    }
    if( proj != nullptr && !one_in( 3 ) ) {
        return; //Less likely for a projectile to deliver enough force
    }

    size_t num_drops = rng( 2, 4 );
    player const *const foe = dynamic_cast<player *>( source );
    if( proj == nullptr && foe != nullptr ) {
        if( foe->weapon.is_cutting_weapon() ) {
            num_drops += rng( 1, 2 );
        }

        if( foe->unarmed_attack() ) {
            damage_instance const burn {
                DT_ACID, static_cast<float>( rng( 1, 5 ) )
            };

            if( one_in( 2 ) ) {
                source->deal_damage( &m, bp_hand_l, burn );
            } else {
                source->deal_damage( &m, bp_hand_r, burn );
            }

            source->add_msg_if_player( m_bad, _( "Acid covering %s burns your hand!" ),
                                       m.disp_name().c_str() );
        }
    }

    const int sx = source == nullptr ? m.posx() : source->posx();
    const int sy = source == nullptr ? m.posy() : source->posy();
    const int dx = sign( sx - m.posx() );
    const int dy = sign( sy - m.posy() );
    bool on_u = false;
    for( size_t i = 0; i < num_drops; i++ ) {
        const int mul = one_in( 2 ) ? 2 : 1;
        tripoint dest( m.posx() + ( dx * mul ) + rng( -1, 1 ),
                       m.posy() + ( dy * mul ) + rng( -1, 1 ),
                       m.posz() );
        g->m.add_field( dest, fd_acid, 1, 0 );
        if( !on_u && dest == g->u.pos() ) {
            on_u = true;
        }
    }

    if( g->u.sees( m.pos() ) ) {
        add_msg( m_warning, _( "Acid sprays out of %s as it is hit!" ),
                 m.disp_name().c_str() );
    }

    if( on_u ) {
        add_msg( m_bad, _( "Some acid lands on you!" ) );
    }
}
