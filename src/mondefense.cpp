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

void mdefense::none( monster *, Creature *, const projectile * )
{
}

void mdefense::zapback( monster *const m, Creature *const source, projectile const *const proj )
{
    // Not a melee attack, attacker lucked out or out of range
    if( source == nullptr || proj != nullptr ||
        rng( 0, 100 ) > m->def_chance || rl_dist( m->pos(), source->pos() ) > 1 ) {
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
    source->deal_damage( m, bp_arm_l, shock );
    source->deal_damage( m, bp_arm_r, shock );

    if( g->u.sees( source->pos() ) ) {
        auto const msg_type = ( source == &g->u ) ? m_bad : m_info;
        add_msg( msg_type, _( "Striking the %s shocks %s!" ),
                 m->name().c_str(), source->disp_name().c_str() );
    }
    source->check_dead_state();
}

static int posp( int posc ) //Determines orientation of target from the source
{
    if( posc > 0 ) {
        return 1;
    }
    if( posc < 0 ) {
        return -1;
    } else {
        return 0;
    }
}

void mdefense::acidsplash( monster *const m, Creature *const source, projectile const *const proj )
{
    if( source == nullptr || m == nullptr ) {
        return;
    }
    if( proj != nullptr && rng( 0, 40 ) > m->def_chance ) {
        return; //Less likely for a projectile to deliver enough force
    }

    int dx = posp( source->posx() - m->posx() );
    int dy = posp( source->posy() - m->posy() );
    if( one_in( 2 ) ) {
        for( int i = 0; i < rng( 3, 5 ); i++ ) {
            const int mul = one_in( 2 ) ? 2 : 1;
            tripoint dest( m->posx() + ( dx * mul ) + rng( -1, 1 ),
                           m->posy() + ( dy * mul ) + rng( -1, 1 ),
                           m->posz() );
            g->m.add_field( dest, fd_acid, rng( 2, 3 ), 0 );
        }
    }
    if( g->u.sees( source->pos() ) ) {
        auto const msg_type = ( source == &g->u ) ? m_bad : m_info;
        add_msg( msg_type, _( "Acid sprays out of %s as it is hit!" ),
                 m->disp_name().c_str(), source->disp_name().c_str() );
    }
}
