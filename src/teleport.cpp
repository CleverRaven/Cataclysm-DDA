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
#include "map_iterator.h"

static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_teleglow( "teleglow" );

static const flag_id json_flag_DIMENSIONAL_ANCHOR( "DIMENSIONAL_ANCHOR" );

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
    //The teleportee is dimensionally anchored so nothing happens
    if( !force && p && ( p->worn_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ||
                         p->has_effect_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ) ) {
        if( display_message ) {
            p->add_msg_if_player( m_warning, _( "You feel a strange, inwards force." ) );
        }
        return false;
    }
    if( p && p->in_vehicle ) {
        here.unboard_vehicle( p->pos() );
    }
    tripoint_abs_ms avatar_pos = get_avatar().get_location();
    bool shifted = false;
    if( !here.inbounds( target ) ) {
        const tripoint_abs_ms abs_ms( here.getabs( target ) );
        g->place_player_overmap( project_to<coords::omt>( abs_ms ), false );
        shifted = true;
        target = here.getlocal( abs_ms );
    }
    //handles teleporting into solids.
    if( here.impassable( target ) ) {
        if( force ) {
            const cata::optional<tripoint> nt =
                random_point( points_in_radius( target, 5 ),
            []( const tripoint & el ) {
                return get_map().passable( el );
            } );
            target = nt ? *nt : target;
        } else {
            if( safe ) {
                if( c_is_u && display_message ) {
                    add_msg( m_bad, _( "You cannot teleport safely." ) );
                }
                if( shifted ) {
                    g->place_player_overmap( project_to<coords::omt>( avatar_pos ), false );
                }
                return false;
            }
            critter.apply_damage( nullptr, bodypart_id( "torso" ), 9999 );
            if( c_is_u ) {
                get_event_bus().send<event_type::teleports_into_wall>( p->getID(), here.obstacle_name( target ) );
                if( display_message ) {
                    add_msg( m_bad, _( "You die after teleporting into a solid." ) );
                }
            }
            critter.check_dead_state();
        }
    }
    //handles telefragging other creatures
    int tfrag_attempts = 5;
    while( Creature *const poor_soul = get_creature_tracker().creature_at<Creature>( target ) ) {
        //Fail if we run out of telefrag attempts
        if( tfrag_attempts-- < 1 ) {
            if( p && display_message ) {
                p->add_msg_player_or_npc( m_warning, _( "You flicker." ), _( "<npcname> flickers." ) );
            } else if( get_player_view().sees( critter ) && display_message ) {
                add_msg( _( "%1$s flickers." ), critter.disp_name() );
            }
            return false;
        }
        Character *const poor_player = dynamic_cast<Character *>( poor_soul );
        if( force ) {
            poor_soul->apply_damage( nullptr, bodypart_id( "torso" ), 9999 );
            poor_soul->check_dead_state();
        } else if( safe ) {
            if( c_is_u && display_message ) {
                add_msg( m_bad, _( "You cannot teleport safely." ) );
            }
            if( shifted ) {
                g->place_player_overmap( project_to<coords::omt>( avatar_pos ), false );
            }
            return false;
        } else if( poor_player && ( poor_player->worn_with_flag( json_flag_DIMENSIONAL_ANCHOR ) ||
                                    poor_player->has_flag( json_flag_DIMENSIONAL_ANCHOR ) ) ) {
            if( display_message ) {
                poor_player->add_msg_if_player( m_warning, _( "You feel disjointed." ) );
            }
            if( shifted ) {
                g->place_player_overmap( project_to<coords::omt>( avatar_pos ), false );
            }
            return false;
        } else {
            const bool poor_soul_is_u = poor_soul->is_avatar();
            if( poor_soul_is_u && display_message ) {
                add_msg( m_bad, _( "…" ) );
                add_msg( m_bad, _( "You explode into thousands of fragments." ) );
            }
            if( p ) {
                if( display_message ) {
                    p->add_msg_player_or_npc( m_warning,
                                              _( "You teleport into %s, and they explode into thousands of fragments." ),
                                              _( "<npcname> teleports into %s, and they explode into thousands of fragments." ),
                                              poor_soul->disp_name() );
                }
                get_event_bus().send<event_type::telefrags_creature>( p->getID(), poor_soul->get_name() );
            } else {
                if( get_player_view().sees( *poor_soul ) ) {
                    if( display_message ) {
                        add_msg( m_good, _( "%1$s teleports into %2$s, killing them!" ),
                                 critter.disp_name(), poor_soul->disp_name() );
                    }
                }
            }
            //Splatter real nice.
            poor_soul->apply_damage( nullptr, bodypart_id( "torso" ), 9999 );
            poor_soul->check_dead_state();
        }
    }
    critter.setpos( target );
    //player and npc exclusive teleporting effects
    if( p ) {
        g->place_player( p->pos() );
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
