#include "mondefense.h"

#include <cstddef>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "ballistics.h"
#include "bodypart.h"
#include "creature.h"
#include "damage.h"
#include "game.h"
#include "messages.h"
#include "monster.h"
#include "player.h"
#include "projectile.h"
#include "rng.h"
#include "translations.h"
#include "enums.h"
#include "item.h"
#include "point.h"

std::vector<tripoint> closest_tripoints_first( int radius, const tripoint &p );

void mdefense::none( monster &, Creature *, const dealt_projectile_attack * )
{
}

void mdefense::zapback( monster &m, Creature *const source,
                        dealt_projectile_attack const *proj )
{
    if( source == nullptr ) {
        return;
    }
    // If we have a projectile, we're a ranged attack, no zapback.
    if( proj != nullptr ) {
        return;
    }

    const player *const foe = dynamic_cast<player *>( source );

    // Players/NPCs can avoid the shock by using non-conductive weapons
    if( foe != nullptr && !foe->weapon.conductive() ) {
        if( foe->reach_attacking ) {
            return;
        }
        if( !foe->used_weapon().is_null() ) {
            return;
        }
    }

    if( source->is_elec_immune() ) {
        return;
    }

    if( g->u.sees( source->pos() ) ) {
        const auto msg_type = source == &g->u ? m_bad : m_info;
        add_msg( msg_type, _( "Striking the %1$s shocks %2$s!" ),
                 m.name(), source->disp_name() );
    }

    damage_instance const shock {
        DT_ELECTRIC, static_cast<float>( rng( 1, 5 ) )
    };
    source->deal_damage( &m, bp_arm_l, shock );
    source->deal_damage( &m, bp_arm_r, shock );

    source->check_dead_state();
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

    size_t num_drops = rng( 4, 6 );
    const player *const foe = dynamic_cast<player *>( source );
    if( proj == nullptr && foe != nullptr ) {
        if( foe->weapon.is_melee( DT_CUT ) || foe->weapon.is_melee( DT_STAB ) ) {
            num_drops += rng( 3, 4 );
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
                                       m.disp_name() );
        }
    }

    const tripoint initial_target = source == nullptr ? m.pos() : source->pos();

    // Don't splatter directly on the `m`, that doesn't work well
    auto pts = closest_tripoints_first( 1, initial_target );
    pts.erase( std::remove( pts.begin(), pts.end(), m.pos() ), pts.end() );

    projectile prj;
    prj.speed = 10;
    prj.range = 4;
    prj.proj_effects.insert( "DRAW_AS_LINE" );
    prj.proj_effects.insert( "NO_DAMAGE_SCALING" );
    prj.impact.add_damage( DT_ACID, rng( 1, 3 ) );
    for( size_t i = 0; i < num_drops; i++ ) {
        const tripoint &target = random_entry( pts );
        projectile_attack( prj, m.pos(), target, { 1200 } );
    }

    if( g->u.sees( m.pos() ) ) {
        add_msg( m_warning, _( "Acid sprays out of %s as it is hit!" ),
                 m.disp_name() );
    }
}
