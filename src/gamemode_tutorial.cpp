#include "gamemode_tutorial.h" // IWYU pragma: associated

#include <array>
#include <cstdlib>
#include <memory>
#include <string>

#include "action.h"
#include "avatar.h"
#include "calendar.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "game.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "optional.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player.h"
#include "pldata.h"
#include "point.h"
#include "profession.h"
#include "scent_map.h"
#include "text_snippets.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const itype_id itype_cig( "cig" );
static const itype_id itype_codeine( "codeine" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_grenade_act( "grenade_act" );
static const itype_id itype_water( "water" );

static const skill_id skill_gun( "gun" );
static const skill_id skill_melee( "melee" );

static const trait_id trait_QUICK( "QUICK" );

static const mtype_id mon_zombie( "mon_zombie" );

namespace io
{

template<>
std::string enum_to_string<tut_lesson>( tut_lesson data )
{
    switch( data ) {
            // *INDENT-OFF*
        case tut_lesson::LESSON_INTRO: return "LESSON_INTRO";
        case tut_lesson::LESSON_MOVE: return "LESSON_MOVE";
        case tut_lesson::LESSON_LOOK: return "LESSON_LOOK";
        case tut_lesson::LESSON_OPEN: return "LESSON_OPEN";
        case tut_lesson::LESSON_CLOSE: return "LESSON_CLOSE";
        case tut_lesson::LESSON_SMASH: return "LESSON_SMASH";
        case tut_lesson::LESSON_WINDOW: return "LESSON_WINDOW";
        case tut_lesson::LESSON_PICKUP: return "LESSON_PICKUP";
        case tut_lesson::LESSON_EXAMINE: return "LESSON_EXAMINE";
        case tut_lesson::LESSON_INTERACT: return "LESSON_INTERACT";
        case tut_lesson::LESSON_FULL_INV: return "LESSON_FULL_INV";
        case tut_lesson::LESSON_WIELD_NO_SPACE: return "LESSON_WIELD_NO_SPACE";
        case tut_lesson::LESSON_AUTOWIELD: return "LESSON_AUTOWIELD";
        case tut_lesson::LESSON_ITEM_INTO_INV: return "LESSON_ITEM_INTO_INV";
        case tut_lesson::LESSON_GOT_ARMOR: return "LESSON_GOT_ARMOR";
        case tut_lesson::LESSON_GOT_WEAPON: return "LESSON_GOT_WEAPON";
        case tut_lesson::LESSON_GOT_FOOD: return "LESSON_GOT_FOOD";
        case tut_lesson::LESSON_GOT_TOOL: return "LESSON_GOT_TOOL";
        case tut_lesson::LESSON_GOT_GUN: return "LESSON_GOT_GUN";
        case tut_lesson::LESSON_GOT_AMMO: return "LESSON_GOT_AMMO";
        case tut_lesson::LESSON_WORE_ARMOR: return "LESSON_WORE_ARMOR";
        case tut_lesson::LESSON_WORE_STORAGE: return "LESSON_WORE_STORAGE";
        case tut_lesson::LESSON_WORE_MASK: return "LESSON_WORE_MASK";
        case tut_lesson::LESSON_WEAPON_INFO: return "LESSON_WEAPON_INFO";
        case tut_lesson::LESSON_HIT_MONSTER: return "LESSON_HIT_MONSTER";
        case tut_lesson::LESSON_PAIN: return "LESSON_PAIN";
        case tut_lesson::LESSON_BUTCHER: return "LESSON_BUTCHER";
        case tut_lesson::LESSON_TOOK_PAINKILLER: return "LESSON_TOOK_PAINKILLER";
        case tut_lesson::LESSON_TOOK_CIG: return "LESSON_TOOK_CIG";
        case tut_lesson::LESSON_DRANK_WATER: return "LESSON_DRANK_WATER";
        case tut_lesson::LESSON_ACT_GRENADE: return "LESSON_ACT_GRENADE";
        case tut_lesson::LESSON_ACT_BUBBLEWRAP: return "LESSON_ACT_BUBBLEWRAP";
        case tut_lesson::LESSON_OVERLOADED: return "LESSON_OVERLOADED";
        case tut_lesson::LESSON_GUN_LOAD: return "LESSON_GUN_LOAD";
        case tut_lesson::LESSON_GUN_FIRE: return "LESSON_GUN_FIRE";
        case tut_lesson::LESSON_RECOIL: return "LESSON_RECOIL";
        case tut_lesson::LESSON_STAIRS: return "LESSON_STAIRS";
        case tut_lesson::LESSON_DARK_NO_FLASH: return "LESSON_DARK_NO_FLASH";
        case tut_lesson::LESSON_DARK: return "LESSON_DARK";
        case tut_lesson::LESSON_PICKUP_WATER: return "LESSON_PICKUP_WATER";
            // *INDENT-ON*
        case tut_lesson::NUM_LESSONS:
            break;
    }
    debugmsg( "Invalid tut_lesson" );
    abort();
}
} // namespace io

bool tutorial_game::init()
{
    // TODO: clean up old tutorial

    // Start at noon
    calendar::turn = calendar::turn_zero + 12_hours;
    tutorials_seen.clear();
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
            starting_om.ter_set( p + tripoint_below, rock );
            // Start with the overmap revealed
            starting_om.seen( p ) = true;
        }
    }
    starting_om.ter_set( lp, oter_id( "tutorial" ) );
    starting_om.ter_set( lp + tripoint_below, oter_id( "tutorial" ) );
    starting_om.clear_mon_groups();

    g->u.toggle_trait( trait_QUICK );
    item lighter( "lighter", 0 );
    lighter.invlet = 'e';
    g->u.inv.add_item( lighter, true, false );
    g->u.set_skill_level( skill_gun, 5 );
    g->u.set_skill_level( skill_melee, 5 );
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
    add_message( tut_lesson::LESSON_INTRO );
    add_message( tut_lesson::LESSON_MOVE );
    add_message( tut_lesson::LESSON_LOOK );

    if( g->light_level( g->u.posz() ) == 1 ) {
        if( g->u.has_amount( itype_flashlight, 1 ) ) {
            add_message( tut_lesson::LESSON_DARK );
        } else {
            add_message( tut_lesson::LESSON_DARK_NO_FLASH );
        }
    }

    if( g->u.get_pain() > 0 ) {
        add_message( tut_lesson::LESSON_PAIN );
    }

    if( g->u.recoil >= MAX_RECOIL ) {
        add_message( tut_lesson::LESSON_RECOIL );
    }

    if( !tutorials_seen[tut_lesson::LESSON_BUTCHER] ) {
        for( const item &it : g->m.i_at( point( g->u.posx(), g->u.posy() ) ) ) {
            if( it.is_corpse() ) {
                add_message( tut_lesson::LESSON_BUTCHER );
                break;
            }
        }
    }

    for( const tripoint &p : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        if( g->m.ter( p ) == t_door_o ) {
            add_message( tut_lesson::LESSON_OPEN );
            break;
        } else if( g->m.ter( p ) == t_door_c ) {
            add_message( tut_lesson::LESSON_CLOSE );
            break;
        } else if( g->m.ter( p ) == t_window ) {
            add_message( tut_lesson::LESSON_SMASH );
            break;
        } else if( g->m.furn( p ) == f_rack && !g->m.i_at( p ).empty() ) {
            add_message( tut_lesson::LESSON_EXAMINE );
            break;
        } else if( g->m.ter( p ) == t_stairs_down ) {
            add_message( tut_lesson::LESSON_STAIRS );
            break;
        } else if( g->m.ter( p ) == t_water_sh ) {
            add_message( tut_lesson::LESSON_PICKUP_WATER );
            break;
        }
    }

    if( !g->m.i_at( point( g->u.posx(), g->u.posy() ) ).empty() ) {
        add_message( tut_lesson::LESSON_PICKUP );
    }
}

void tutorial_game::pre_action( action_id &act )
{
    switch( act ) {
        case ACTION_SAVE:
        case ACTION_QUICKSAVE:
            popup( _( "You're saving a tutorial - the tutorial world lacks certain features of normal worlds.  "
                      "Weird things might happen when you load this save.  You have been warned." ) );
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
            if( g->u.weapon.is_gun() && !tutorials_seen[tut_lesson::LESSON_GUN_FIRE] ) {
                g->place_critter_at( mon_zombie, tripoint( g->u.posx(), g->u.posy() - 6, g->u.posz() ) );
                g->place_critter_at( mon_zombie, tripoint( g->u.posx() + 2, g->u.posy() - 5, g->u.posz() ) );
                g->place_critter_at( mon_zombie, tripoint( g->u.posx() - 2, g->u.posy() - 5, g->u.posz() ) );
                add_message( tut_lesson::LESSON_GUN_FIRE );
            }
            break;

        case ACTION_OPEN:
            add_message( tut_lesson::LESSON_CLOSE );
            break;

        case ACTION_CLOSE:
            add_message( tut_lesson::LESSON_SMASH );
            break;

        case ACTION_USE:
            if( g->u.has_amount( itype_grenade_act, 1 ) ) {
                add_message( tut_lesson::LESSON_ACT_GRENADE );
            }
            for( const tripoint &dest : g->m.points_in_radius( g->u.pos(), 1 ) ) {
                if( g->m.tr_at( dest ).id == trap_str_id( "tr_bubblewrap" ) ) {
                    add_message( tut_lesson::LESSON_ACT_BUBBLEWRAP );
                }
            }
            break;

        case ACTION_EAT:
            if( g->u.last_item == itype_codeine ) {
                add_message( tut_lesson::LESSON_TOOK_PAINKILLER );
            } else if( g->u.last_item == itype_cig ) {
                add_message( tut_lesson::LESSON_TOOK_CIG );
            } else if( g->u.last_item == itype_water ) {
                add_message( tut_lesson::LESSON_DRANK_WATER );
            }
            break;

        case ACTION_WEAR: {
            item it( g->u.last_item, 0 );
            if( it.is_armor() ) {
                if( it.get_coverage() >= 2 || it.get_thickness() >= 2 ) {
                    add_message( tut_lesson::LESSON_WORE_ARMOR );
                }
                if( it.get_env_resist() >= 2 ) {
                    add_message( tut_lesson::LESSON_WORE_MASK );
                }
            }
        }
        break;

        case ACTION_WIELD:
            if( g->u.weapon.is_gun() ) {
                add_message( tut_lesson::LESSON_GUN_LOAD );
            }
            break;

        case ACTION_EXAMINE:
            add_message( tut_lesson::LESSON_INTERACT );
        /* fallthrough */
        case ACTION_PICKUP: {
            item it( g->u.last_item, 0 );
            if( it.is_armor() ) {
                add_message( tut_lesson::LESSON_GOT_ARMOR );
            } else if( it.is_gun() ) {
                add_message( tut_lesson::LESSON_GOT_GUN );
            } else if( it.is_ammo() ) {
                add_message( tut_lesson::LESSON_GOT_AMMO );
            } else if( it.is_tool() ) {
                add_message( tut_lesson::LESSON_GOT_TOOL );
            } else if( it.is_food() ) {
                add_message( tut_lesson::LESSON_GOT_FOOD );
            } else if( it.is_melee() ) {
                add_message( tut_lesson::LESSON_GOT_WEAPON );
            }

        }
        break;

        default:
            // TODO: add more actions here
            break;

    }
}

void tutorial_game::add_message( tut_lesson lesson )
{
    if( tutorials_seen[lesson] ) {
        return;
    }
    tutorials_seen[lesson] = true;
    popup( SNIPPET.get_snippet_by_id( snippet_id( io::enum_to_string<tut_lesson>( lesson ) ) ).value_or(
               translation() ).translated(), PF_ON_TOP );
    g->refresh_all();
}
