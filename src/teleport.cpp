#include "teleport.h"

#include <cmath>
#include <memory>
#include <string>

#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "point.h"
#include "rng.h"
#include "translations.h"
#include "type_id.h"
#include "viewer.h"

static const flag_id json_flag_DIMENSIONAL_ANCHOR( "DIMENSIONAL_ANCHOR" );

static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_teleglow( "teleglow" );

bool teleport::teleport( Creature &critter, int min_distance, int max_distance, bool safe,
                         bool add_teleglow )
{
    if( min_distance > max_distance ) {
        debugmsg( "ERROR: Function teleport::teleport called with invalid arguments." );
        return false;
    }
    Character *const p = critter.as_character();
    const bool c_is_u = p != nullptr && p->is_avatar();
    int tries = 0;
    tripoint origin = critter.pos();
    tripoint new_pos = origin;
    map &here = get_map();
    //The teleportee is dimensionally anchored so nothing happens
    if( p && ( p->worn_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ||
               p->has_flag( json_flag_DIMENSIONAL_ANCHOR ) ) ) {
        p->add_msg_if_player( m_warning, _( "You feel a strange, inwards force." ) );
        return false;
    }
    do {
        int rangle = rng( 0, 360 );
        int rdistance = rng( min_distance, max_distance );
        new_pos.x = origin.x + rdistance * std::cos( rangle );
        new_pos.y = origin.y + rdistance * std::sin( rangle );
        tries++;
    } while( here.impassable( new_pos ) && tries < 20 );
    //handles teleporting into solids.
    if( here.impassable( new_pos ) ) {
        if( safe ) {
            if( c_is_u ) {
                add_msg( m_bad, _( "You cannot teleport safely." ) );
            }
            return false;
        }
        critter.apply_damage( nullptr, bodypart_id( "torso" ), 9999 );
        if( c_is_u ) {
            get_event_bus().send<event_type::teleports_into_wall>( p->getID(), here.obstacle_name( new_pos ) );
            add_msg( m_bad, _( "You die after teleporting into a solid." ) );
        }
        critter.check_dead_state();

    }
    //handles telefragging other creatures
    if( Creature *const poor_soul = g->critter_at<Creature>( new_pos ) ) {
        Character *const poor_player = dynamic_cast<Character *>( poor_soul );
        if( safe ) {
            if( c_is_u ) {
                add_msg( m_bad, _( "You cannot teleport safely." ) );
            }
            return false;
        } else if( poor_player && ( poor_player->worn_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ||
                                    poor_player->has_flag( json_flag_DIMENSIONAL_ANCHOR ) ) ) {
            poor_player->add_msg_if_player( m_warning, _( "You feel disjointed." ) );
            return false;
        } else {
            const bool poor_soul_is_u = poor_soul->is_avatar();
            if( poor_soul_is_u ) {
                add_msg( m_bad, _( "…" ) );
                add_msg( m_bad, _( "You explode into thousands of fragments." ) );
            }
            if( p ) {
                p->add_msg_player_or_npc( m_warning,
                                          _( "You teleport into %s, and they explode into thousands of fragments." ),
                                          _( "<npcname> teleports into %s, and they explode into thousands of fragments." ),
                                          poor_soul->disp_name() );
                get_event_bus().send<event_type::telefrags_creature>( p->getID(), poor_soul->get_name() );
            } else {
                if( get_player_view().sees( *poor_soul ) ) {
                    add_msg( m_good, _( "%1$s teleports into %2$s, killing them!" ),
                             critter.disp_name(), poor_soul->disp_name() );
                }
            }
            //Splatter real nice.
            poor_soul->apply_damage( nullptr, bodypart_id( "torso" ), 9999 );
            poor_soul->check_dead_state();
        }
    }

    critter.setpos( new_pos );
    //player and npc exclusive teleporting effects
    if( p ) {
        if( add_teleglow ) {
            p->add_effect( effect_teleglow, 30_minutes );
        }
    }
    if( c_is_u ) {
        g->update_map( *p );
    }
    critter.remove_effect( effect_grabbed );
    return true;
}
