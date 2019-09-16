#include "tutorial.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "avatar.h"
#include "coordinate_conversions.h"
#include "game.h"
#include "gamemode.h"
#include "json.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"
#include "profession.h"
#include "scent_map.h"
#include "translations.h"
#include "trap.h"
#include "calendar.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "pldata.h"
#include "units.h"
#include "translations.h"
#include "type_id.h"
#include "point.h"
#include "weather.h"

const mtype_id mon_zombie( "mon_zombie" );

static std::vector<translation> tut_text;

bool tutorial_game::init()
{
    // TODO: clean up old tutorial

    calendar::turn = calendar::turn_zero + 12_hours; // Start at noon
    for( auto &elem : tutorials_seen ) {
        elem = false;
    }
    g->scent.reset();
    g->weather.temperature = 65;
    // We use a Z-factor of 10 so that we don't plop down tutorial rooms in the
    // middle of the "real" game world
    g->u.normalize();
    g->u.str_cur = g->u.str_max;
    g->u.per_cur = g->u.per_max;
    g->u.int_cur = g->u.int_max;
    g->u.dex_cur = g->u.dex_max;

    for( int i = 0; i < num_hp_parts; i++ ) {
        g->u.hp_cur[i] = g->u.hp_max[i];
    }

    const oter_id rock( "rock" );
    //~ default name for the tutorial
    g->u.name = _( "John Smith" );
    g->u.prof = profession::generic();
    // overmap terrain coordinates
    const tripoint lp( 50, 50, 0 );
    auto &starting_om = overmap_buffer.get( point_zero );
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint p( i, j, 0 );
            starting_om.ter( p + tripoint_below ) = rock;
            // Start with the overmap revealed
            starting_om.seen( p ) = true;
        }
    }
    starting_om.ter( lp ) = oter_id( "tutorial" );
    starting_om.ter( lp + tripoint_below ) = oter_id( "tutorial" );
    starting_om.clear_mon_groups();

    g->u.toggle_trait( trait_id( "QUICK" ) );
    item lighter( "lighter", 0 );
    lighter.invlet = 'e';
    g->u.inv.add_item( lighter, true, false );
    g->u.set_skill_level( skill_id( "gun" ), 5 );
    g->u.set_skill_level( skill_id( "melee" ), 5 );
    g->load_map( omt_to_sm_copy( lp ) );
    g->u.setx( 2 );
    g->u.sety( 4 );

    // This shifts the view to center the players pos
    g->update_map( g->u );
    return true;
}

void tutorial_game::per_turn()
{
    // note that add_message does nothing if the message was already shown
    add_message( LESSON_INTRO );
    add_message( LESSON_MOVE );
    add_message( LESSON_LOOK );

    if( g->light_level( g->u.posz() ) == 1 ) {
        if( g->u.has_amount( "flashlight", 1 ) ) {
            add_message( LESSON_DARK );
        } else {
            add_message( LESSON_DARK_NO_FLASH );
        }
    }

    if( g->u.get_pain() > 0 ) {
        add_message( LESSON_PAIN );
    }

    if( g->u.recoil >= MAX_RECOIL ) {
        add_message( LESSON_RECOIL );
    }

    if( !tutorials_seen[LESSON_BUTCHER] ) {
        for( const item &it : g->m.i_at( point( g->u.posx(), g->u.posy() ) ) ) {
            if( it.is_corpse() ) {
                add_message( LESSON_BUTCHER );
                break;
            }
        }
    }

    for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        if( g->m.ter( p ) == t_door_o ) {
            add_message( LESSON_OPEN );
            break;
        } else if( g->m.ter( p ) == t_door_c ) {
            add_message( LESSON_CLOSE );
            break;
        } else if( g->m.ter( p ) == t_window ) {
            add_message( LESSON_SMASH );
            break;
        } else if( g->m.furn( p ) == f_rack && !g->m.i_at( p ).empty() ) {
            add_message( LESSON_EXAMINE );
            break;
        } else if( g->m.ter( p ) == t_stairs_down ) {
            add_message( LESSON_STAIRS );
            break;
        } else if( g->m.ter( p ) == t_water_sh ) {
            add_message( LESSON_PICKUP_WATER );
            break;
        }
    }

    if( !g->m.i_at( point( g->u.posx(), g->u.posy() ) ).empty() ) {
        add_message( LESSON_PICKUP );
    }
}

void tutorial_game::pre_action( action_id &act )
{
    switch( act ) {
        case ACTION_SAVE:
        case ACTION_QUICKSAVE:
            popup( _( "You're saving a tutorial - the tutorial world lacks certain features of normal worlds. "
                      "Weird things might happen when you load this save. You have been warned." ) );
            break;
        default:
            // Other actions are fine.
            break;
    }
}

void tutorial_game::post_action( action_id act )
{
    switch( act ) {
        case ACTION_RELOAD_WEAPON:
            if( g->u.weapon.is_gun() && !tutorials_seen[LESSON_GUN_FIRE] ) {
                g->summon_mon( mon_zombie, tripoint( g->u.posx(), g->u.posy() - 6, g->u.posz() ) );
                g->summon_mon( mon_zombie, tripoint( g->u.posx() + 2, g->u.posy() - 5, g->u.posz() ) );
                g->summon_mon( mon_zombie, tripoint( g->u.posx() - 2, g->u.posy() - 5, g->u.posz() ) );
                add_message( LESSON_GUN_FIRE );
            }
            break;

        case ACTION_OPEN:
            add_message( LESSON_CLOSE );
            break;

        case ACTION_CLOSE:
            add_message( LESSON_SMASH );
            break;

        case ACTION_USE:
            if( g->u.has_amount( "grenade_act", 1 ) ) {
                add_message( LESSON_ACT_GRENADE );
            }
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 1 ) ) {
                if( g->m.tr_at( dest ).id == trap_str_id( "tr_bubblewrap" ) ) {
                    add_message( LESSON_ACT_BUBBLEWRAP );
                }
            }
            break;

        case ACTION_EAT:
            if( g->u.last_item == "codeine" ) {
                add_message( LESSON_TOOK_PAINKILLER );
            } else if( g->u.last_item == "cig" ) {
                add_message( LESSON_TOOK_CIG );
            } else if( g->u.last_item == "water" ) {
                add_message( LESSON_DRANK_WATER );
            }
            break;

        case ACTION_WEAR: {
            item it( g->u.last_item, 0 );
            if( it.is_armor() ) {
                if( it.get_coverage() >= 2 || it.get_thickness() >= 2 ) {
                    add_message( LESSON_WORE_ARMOR );
                }
                if( it.get_storage() >= units::from_liter( 5 ) ) {
                    add_message( LESSON_WORE_STORAGE );
                }
                if( it.get_env_resist() >= 2 ) {
                    add_message( LESSON_WORE_MASK );
                }
            }
        }
        break;

        case ACTION_WIELD:
            if( g->u.weapon.is_gun() ) {
                add_message( LESSON_GUN_LOAD );
            }
            break;

        case ACTION_EXAMINE:
            add_message( LESSON_INTERACT );
        /* fallthrough */
        case ACTION_PICKUP: {
            item it( g->u.last_item, 0 );
            if( it.is_armor() ) {
                add_message( LESSON_GOT_ARMOR );
            } else if( it.is_gun() ) {
                add_message( LESSON_GOT_GUN );
            } else if( it.is_ammo() ) {
                add_message( LESSON_GOT_AMMO );
            } else if( it.is_tool() ) {
                add_message( LESSON_GOT_TOOL );
            } else if( it.is_food() ) {
                add_message( LESSON_GOT_FOOD );
            } else if( it.is_melee() ) {
                add_message( LESSON_GOT_WEAPON );
            }

        }
        break;

        default: // TODO: add more actions here
            break;

    }
}

void tutorial_game::add_message( tut_lesson lesson )
{
    if( tutorials_seen[lesson] ) {
        return;
    }
    tutorials_seen[lesson] = true;
    popup( tut_text[lesson].translated(), PF_ON_TOP );
    g->refresh_all();
}

void load_tutorial_messages( JsonObject &jo )
{
    // loading them all at once, as they have to be in exact order
    tut_text.clear();
    JsonArray messages = jo.get_array( "messages" );
    while( messages.has_more() ) {
        translation next;
        messages.read_next( next );
        tut_text.emplace_back( next );
    }
}

void clear_tutorial_messages()
{
    tut_text.clear();
}
