#include "teleport.h"

#include "character.h"
#include "avatar.h"
#include "creature.h"
#include "player.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "point.h"


const efftype_id effect_teleglow( "teleglow" );

bool teleport::teleport( Creature &critter, int min_distance, int max_distance, bool safe,
                         bool add_teleglow )
{
    if( critter == nullptr || min_distance > max_distance ) {
        debugmsg( "ERROR: Function teleport::teleport called with invalid arguments." );
        return false;
    }

    const bool c_is_u = &critter == &g->u;
    player *const p = critter->as_player();
    int tries = 0;
    tripoint origin = critter->pos();
    tripoint new_pos = tripoint_zero;
    do {
        int rangle = rng( 0, 360 );
        int rdistance = rng( min_distance, max_distance );
        new_pos.x = origin.x + rdistance * cos( rangle );
        new_pos.y = origin.y + rdistance * sin( rangle );
        tries++;
    } while( g->m.impassable( new_pos ) && tries < 20 );
    //handles teleporting into solids.
    if( g->m.impassable( new_pos ) ) {
        if( safe ) {
            if( c_is_u ) {
                add_msg( m_bad, _( "You cannot teleport safely" ) );
            }
            return false;
        } else {
            critter->apply_damage( nullptr, bp_torso, 9999 );
            if( c_is_u ) {
                g->events().send<event_type::teleports_into_wall>( p->getID(), g->m.obstacle_name( new_pos ) );
                add_msg( m_bad, _( "You die after teleporting into a solid." ) );
            }
            critter->check_dead_state();
        }
    }
    //handles telefragging other creatures
    if( Creature *const poor_soul = g->critter_at<Creature>( new_pos ) ) {
        if( safe ) {
            if( c_is_u ) {
                add_msg( m_bad, _( "You cannot teleport safely." ) );
            }
            return false;
        } else {
            const bool poor_soul_is_u = ( poor_soul == &g->u );
            if( poor_soul_is_u ) {
                add_msg( m_bad, _( "..." ) );
                add_msg( m_bad, _( "You exlpode into thousands of fragments." ) );
            }
            if( p ) {
                p->add_msg_player_or_npc( m_warning,
                                          _( "You teleport into %s, and they explode into thousands of fragments." ),
                                          _( "<npcname> teleports into %s, and they explode into thousands of fragments." ),
                                          poor_soul->disp_name() );
                g->events().send<event_type::telefrags_creature>( p->getID(), poor_soul->get_name() );
            } else {
                if( g->u.sees( *poor_soul ) ) {
                    add_msg( m_good, _( "%1$s teleports into %2$s, killing them!" ),
                             critter->disp_name(), poor_soul->disp_name() );
                }
            }
            poor_soul->apply_damage( nullptr, bp_torso, 9999 ); //Splatter real nice.
            poor_soul->check_dead_state();
        }
    }

    critter->setpos( new_pos );
    //player and npc exclusive teleporting effects
    if( p ) {
        if( add_teleglow ) {
            p->add_effect( effect_teleglow, 30_minutes );
        }
    }
    if( c_is_u ) {
        g->update_map( *p );
    }
    return true;
}
