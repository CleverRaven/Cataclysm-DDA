#include "teleport.h"

#include "character.h"
#include "avatar.h"
#include "creature.h"
#include "player.h"
#include "monster.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "point.h"


const efftype_id effect_teleglow( "teleglow" );

bool teleport::teleport( Creature *c, int min_distance, int max_distance, bool safe,
                         bool add_teleglow )
{
    if( c == nullptr || min_distance > max_distance ) {
        debugmsg( "ERROR: Function teleport::teleport called with invalid arguments." );
        return false;
    }

    bool c_is_u = ( c == &g->u );
    player *p = dynamic_cast<player *>( c );
    int tries = 0;
    tripoint origin = c->pos();
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
            c->apply_damage( nullptr, bp_torso, 9999 );
            if( c_is_u ) {
                g->events().send<event_type::teleports_into_wall>( p->getID(), g->m.obstacle_name( new_pos ) );
                add_msg( m_bad, _( "You die after teleporting within a solid" ) );
            }
        }
    }
    //handles telefragging other creatures
    if( Creature *const poor_soul = g->critter_at<Creature>( new_pos ) ) {
        if( safe ) {
            if( c_is_u ) {
                add_msg( m_bad, _( "You cannot teleport safely" ) );
            }
            return false;
        } else {
            const bool poor_soul_is_u = ( poor_soul == &g->u );
            if( poor_soul_is_u ) {
                add_msg( m_bad, _( "..." ) );
                add_msg( m_bad, _( "You exlpode into thousands of fragments." ) );
            }
            if( p ) {
                p->add_msg_player_or_npc( m_bad,
                                          _( "You teleport into %s, and they explode into thousands of fragments." ),
                                          _( "<npcname> teleports into %s, and they explode into thousands of fragments." ),
                                          poor_soul->disp_name() );
                g->events().send<event_type::telefrags_creature>( p->getID(), poor_soul->get_name() );
            } else {
                if( g->u.sees( poor_soul->pos() ) ) {
                    add_msg( m_good, _( "%1$s teleports into %2$s, killing them!" ),
                             c->disp_name(), poor_soul->disp_name() );
                }
            }
            poor_soul->apply_damage( nullptr, bp_torso, 9999 ); //Splatter real nice.
            poor_soul->check_dead_state();
        }
    }

    c->setpos( new_pos );
    //player and npc exclusive teleporting effects
    if( p ) {
        if( add_teleglow ) {
            add_msg( m_bad, _( "unsafe" ) );
            p->add_effect( effect_teleglow, 30_minutes );
        }
    }
    if( c_is_u ) {
        g->update_map( *p );
    }
    return true;
}
