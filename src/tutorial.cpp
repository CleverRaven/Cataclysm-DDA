#include "tutorial.h"

#include "action.h"
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

const mtype_id mon_zombie( "mon_zombie" );

std::vector<std::string> tut_text;

bool tutorial_game::init()
{
    // TODO: clean up old tutorial

    calendar::turn = HOURS( 12 ); // Start at noon
    for( auto &elem : tutorials_seen ) {
        elem = false;
    }
    g->scent.reset();
    g->temperature = 65;
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
    int lx = 50, ly = 50; // overmap terrain coordinates
    auto &starting_om = overmap_buffer.get( 0, 0 );
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            starting_om.ter( i, j, -1 ) = rock;
            // Start with the overmap revealed
            starting_om.seen( i, j, 0 ) = true;
        }
    }
    starting_om.ter( lx, ly, 0 ) = oter_id( "tutorial" );
    starting_om.ter( lx, ly, -1 ) = oter_id( "tutorial" );
    starting_om.clear_mon_groups();

    g->u.toggle_trait( trait_id( "QUICK" ) );
    item lighter( "lighter", 0 );
    lighter.invlet = 'e';
    g->u.inv.add_item( lighter, true, false );
    g->u.set_skill_level( skill_id( "gun" ), 5 );
    g->u.set_skill_level( skill_id( "melee" ), 5 );
    g->load_map( omt_to_sm_copy( tripoint( lx, ly, 0 ) ) );
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
        for( size_t i = 0; i < g->m.i_at( g->u.posx(), g->u.posy() ).size(); i++ ) {
            if( g->m.i_at( g->u.posx(), g->u.posy() )[i].is_corpse() ) {
                add_message( LESSON_BUTCHER );
                i = g->m.i_at( g->u.posx(), g->u.posy() ).size();
            }
        }
    }

    bool showed_message = false;
    for( int x = g->u.posx() - 1; x <= g->u.posx() + 1 && !showed_message; x++ ) {
        for( int y = g->u.posy() - 1; y <= g->u.posy() + 1 && !showed_message; y++ ) {
            if( g->m.ter( x, y ) == t_door_o ) {
                add_message( LESSON_OPEN );
                showed_message = true;
            } else if( g->m.ter( x, y ) == t_door_c ) {
                add_message( LESSON_CLOSE );
                showed_message = true;
            } else if( g->m.ter( x, y ) == t_window ) {
                add_message( LESSON_SMASH );
                showed_message = true;
            } else if( g->m.furn( x, y ) == f_rack && !g->m.i_at( x, y ).empty() ) {
                add_message( LESSON_EXAMINE );
                showed_message = true;
            } else if( g->m.ter( x, y ) == t_stairs_down ) {
                add_message( LESSON_STAIRS );
                showed_message = true;
            } else if( g->m.ter( x, y ) == t_water_sh ) {
                add_message( LESSON_PICKUP_WATER );
                showed_message = true;
            }
        }
    }

    if( !g->m.i_at( g->u.posx(), g->u.posy() ).empty() ) {
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
            act = ACTION_NULL;
            break;
        default:
            // Other actions are fine.
            break;
    }
}

void tutorial_game::post_action( action_id act )
{
    switch( act ) {
        case ACTION_RELOAD:
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

        default: //TODO: add more actions here
            break;

    }
}

void tutorial_game::add_message( tut_lesson lesson )
{
    if( tutorials_seen[lesson] ) {
        return;
    }
    tutorials_seen[lesson] = true;
    popup( tut_text[lesson], PF_ON_TOP );
    g->refresh_all();
}

void load_tutorial_messages( JsonObject &jo )
{
    // loading them all at once, as they have to be in exact order
    tut_text.clear();
    JsonArray messages = jo.get_array( "messages" );
    while( messages.has_more() ) {
        tut_text.push_back( _( messages.next_string().c_str() ) );
    }
}

void clear_tutorial_messages()
{
    tut_text.clear();
}
