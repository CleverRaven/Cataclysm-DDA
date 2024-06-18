#include "teleport.h"

#include <cmath>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "effect.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "game.h"
#include "map.h"
#include "messages.h"
#include "point.h"
#include "rng.h"
#include "translations.h"
#include "type_id.h"
#include "viewer.h"
#include "map_iterator.h"

static const efftype_id effect_teleglow( "teleglow" );

static const flag_id json_flag_DIMENSIONAL_ANCHOR( "DIMENSIONAL_ANCHOR" );
static const flag_id json_flag_GRAB( "GRAB" );
static const flag_id json_flag_TELEPORT_LOCK( "TELEPORT_LOCK" );

bool teleport::teleport( Creature &critter, int min_distance, int max_distance, bool safe,
                         bool add_teleglow )
{
    if( min_distance > max_distance ) {
        debugmsg( "ERROR: Function teleport::teleport called with invalid arguments." );
        return false;
    }
    int tries = 0;
    tripoint origin = critter.pos();
    tripoint new_pos = origin;
    map &here = get_map();
    do {
        int rangle = rng( 0, 360 );
        int rdistance = rng( min_distance, max_distance );
        new_pos.x = origin.x + rdistance * std::cos( rangle );
        new_pos.y = origin.y + rdistance * std::sin( rangle );
        tries++;
    } while( here.impassable( new_pos ) && tries < 20 );
    return teleport_to_point( critter, new_pos, safe, add_teleglow );
}

bool teleport::teleport_to_point( Creature &critter, tripoint target, bool safe,
                                  bool add_teleglow, bool display_message, bool force )
{
    if( critter.pos() == target ) {
        return false;
    }
    Character *const p = critter.as_character();
    const bool c_is_u = p != nullptr && p->is_avatar();
    map &here = get_map();
    tripoint_abs_ms abs_ms( here.getabs( target ) );
    //The teleportee is dimensionally anchored so nothing happens
    if( !force && p && ( p->worn_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ||
                         p->has_effect_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ||
                         p->has_effect_with_flag( json_flag_TELEPORT_LOCK ) ) ) {
        if( display_message ) {
            p->add_msg_if_player( m_warning, _( "You feel a strange, inwards force." ) );
        }
        return false;
    }
    if( p && p->in_vehicle ) {
        here.unboard_vehicle( p->pos() );
    }
    map tm;
    map *dest = &here;
    tripoint dest_target = target;
    if( !here.inbounds( target ) ) {
        if( c_is_u ) {
            g->place_player_overmap( project_to<coords::omt>( abs_ms ), false );
        } else {
            dest = &tm;
            dest->load( project_to<coords::sm>( abs_ms ), false );
            dest->spawn_monsters( true, true );
        }
        dest_target = dest->getlocal( abs_ms );
    }
    //handles teleporting into solids.
    if( dest->impassable( dest_target ) ) {
        if( force ) {
            const std::optional<tripoint> nt =
                random_point( points_in_radius( dest_target, 5 ),
            [dest]( const tripoint & el ) {
                return dest->passable( el );
            } );
            dest_target = nt ? *nt : dest_target;
        } else {
            if( safe ) {
                if( c_is_u && display_message ) {
                    add_msg( m_bad, _( "You cannot teleport safely." ) );
                }
                return false;
            }
            critter.apply_damage( nullptr, bodypart_id( "torso" ), 9999 );
            if( c_is_u ) {
                get_event_bus().send<event_type::teleports_into_wall>( p->getID(),
                        dest->obstacle_name( dest_target ) );
                if( display_message ) {
                    add_msg( m_bad, _( "You die after teleporting into a solid." ) );
                }
            }
            critter.check_dead_state();
        }
    }
    //update pos
    abs_ms = dest->getglobal( dest_target );
    target = here.getlocal( abs_ms );
    //handles telefragging other creatures
    int tfrag_attempts = 5;
    bool collision = false;
    int collision_angle = 0;
    while( Creature *const poor_soul = get_creature_tracker().creature_at<Creature>( abs_ms ) ) {
        //Fail if we run out of telefrag attempts
        if( tfrag_attempts-- < 1 ) {
            if( p && display_message ) {
                p->add_msg_player_or_npc( m_warning, _( "You flicker." ), _( "<npcname> flickers." ) );
            } else if( get_player_view().sees( critter ) && display_message ) {
                add_msg( _( "%1$s flickers." ), critter.disp_name() );
            }
            return false;
        }
        //if the thing that was going to be teleported into has a dimensional anchor, break out early and don't teleport.
        if( poor_soul->as_character() &&
            ( poor_soul->as_character()->worn_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ||
              poor_soul->as_character()->has_effect_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ) ) {
            poor_soul->as_character()->add_msg_if_player( m_warning, _( "You feel disjointed." ) );
            return false;
        }
        if( force ) {
            //this should only happen through debug menu, so this won't affect the player.
            poor_soul->apply_damage( nullptr, bodypart_id( "torso" ), 9999 );
            poor_soul->check_dead_state();
        } else if( safe ) {
            if( c_is_u && display_message ) {
                add_msg( m_bad, _( "You cannot teleport safely." ) );
            }
            return false;
        } else if( !collision ) {
            //we passed all the conditions needed for a teleport accident, so handle messages for teleport accidents here
            const bool poor_soul_is_u = poor_soul->is_avatar();
            if( poor_soul_is_u && display_message ) {
                add_msg( m_bad, _( "You're blasted with strange energy!" ) );
            }
            if( p ) {
                if( display_message ) {
                    p->add_msg_player_or_npc( m_warning,
                                              _( "You collide with %s mid teleport, and you are both knocked away by a violent explosion of energy." ),
                                              _( "<npcname> collides with %s mid teleport, and they are both knocked away by a violent explosion of energy." ),
                                              poor_soul->disp_name() );
                }
            } else {
                if( get_player_view().sees( *poor_soul ) ) {
                    if( display_message ) {
                        add_msg( m_warning,
                                 _( "%1$s collides with %2$s mid teleport, and they are both knocked away by a violent explosion of energy!" ),
                                 critter.disp_name(), poor_soul->disp_name() );
                    }
                }
                //once collision this if block shouldn't run so everything here should only happen once
                collision = true;
                //determine a random angle to throw the thing it teleported into, then fling it.
                collision_angle = rng( 0, 360 );
                g->fling_creature( poor_soul, units::from_degrees( collision_angle - 180 ), 40, false, true );
                //spawn a mostly cosmetic explosion for flair.
                explosion_handler::explosion( &critter, target, 10 );
                //if it was grabbed, it isn't anymore.
                for( const effect &grab : poor_soul->get_effects_with_flag( json_flag_GRAB ) ) {
                    poor_soul->remove_effect( grab.get_id() );
                }
                //apply a bunch of damage to it, similar to a tear in reality
                poor_soul->apply_damage( nullptr, bodypart_id( "arm_l" ), rng( 5, 10 ) );
                poor_soul->apply_damage( nullptr, bodypart_id( "arm_r" ), rng( 5, 10 ) );
                poor_soul->apply_damage( nullptr, bodypart_id( "leg_l" ), rng( 7, 12 ) );
                poor_soul->apply_damage( nullptr, bodypart_id( "leg_r" ), rng( 7, 12 ) );
                poor_soul->apply_damage( nullptr, bodypart_id( "torso" ), rng( 5, 15 ) );
                poor_soul->apply_damage( nullptr, bodypart_id( "head" ), rng( 2, 8 ) );
                poor_soul->check_dead_state();
            }
        }
    }
    critter.move_to( abs_ms );
    //there was a collision with a creature at some point, so handle that.
    if( collision ) {
        //throw the thing that teleported in the opposite direction as the thing it teleported into.
        g->fling_creature( &critter, units::from_degrees( collision_angle - 180 ), 40, false, true );
        //do a bunch of damage to it too.
        critter.apply_damage( nullptr, bodypart_id( "arm_l" ), rng( 5, 10 ) );
        critter.apply_damage( nullptr, bodypart_id( "arm_r" ), rng( 5, 10 ) );
        critter.apply_damage( nullptr, bodypart_id( "leg_l" ), rng( 7, 12 ) );
        critter.apply_damage( nullptr, bodypart_id( "leg_r" ), rng( 7, 12 ) );
        critter.apply_damage( nullptr, bodypart_id( "torso" ), rng( 5, 15 ) );
        critter.apply_damage( nullptr, bodypart_id( "head" ), rng( 2, 8 ) );
        critter.check_dead_state();
    }
    //player and npc exclusive teleporting effects
    if( p && add_teleglow ) {
        p->add_effect( effect_teleglow, 30_minutes );
    }
    if( c_is_u ) {
        g->place_player( p->pos() );
        g->update_map( *p );
    }
    for( const effect &grab : critter.get_effects_with_flag( json_flag_GRAB ) ) {
        critter.remove_effect( grab.get_id() );
    }
    return true;
}
