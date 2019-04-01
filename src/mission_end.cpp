#include "mission.h" // IWYU pragma: associated

#include "debug.h"
#include "game.h"
#include "messages.h"
#include "npc.h"
#include "output.h"
#include "rng.h"
#include "translations.h"
#include "map.h"
#include "overmapbuffer.h"

const efftype_id effect_infection( "infection" );

void mission_end::heal_infection( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    p->remove_effect( effect_infection );
}

void mission_end::leave( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    p->set_attitude( NPCATT_NULL );
}

void mission_end::thankful( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    if( p->get_attitude() == NPCATT_MUG || p->get_attitude() == NPCATT_WAIT_FOR_LEAVE ||
        p->get_attitude() == NPCATT_FLEE || p->get_attitude() == NPCATT_KILL ||
        p->get_attitude() == NPCATT_FLEE_TEMP ) {
        p->set_attitude( NPCATT_NULL );
    }
    if( p->chatbin.first_topic != "TALK_FRIEND" ) {
        p->chatbin.first_topic = "TALK_STRANGER_FRIENDLY";
    }
    p->personality.aggression -= 1;
}

void mission_end::deposit_box( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    p->set_attitude( NPCATT_NULL );//npc leaves your party
    std::string itemName = "deagle_44";
    if( one_in( 4 ) ) {
        itemName = "katana";
    } else if( one_in( 3 ) ) {
        itemName = "m4a1";
    }
    g->u.i_add( item( itemName, 0 ) );
    add_msg( m_good, _( "%s gave you an item from the deposit box." ), p->name.c_str() );
}

void mission_end::evac_construct_5( mission *miss )
{
    const int EVAC_CENTER_SIZE = 5;

    tripoint site = mission_util::target_om_ter_random( "evac_center_19", 1, miss, false,
                    EVAC_CENTER_SIZE );
    tinymap storageRoom;
    storageRoom.load( site.x * 2, site.y * 2, site.z, false );

    char direction;
    if( overmap_buffer.find_closest( site, "evac_center_19_north", 1,
                                     true ) != overmap::invalid_tripoint ) {
        direction = 'N';
    } else if( overmap_buffer.find_closest( site, "evac_center_19_east", 1,
                                            true ) != overmap::invalid_tripoint ) {
        direction = 'E';
    } else if( overmap_buffer.find_closest( site, "evac_center_19_south", 1,
                                            true ) != overmap::invalid_tripoint ) {
        direction = 'S';
    } else if( overmap_buffer.find_closest( site, "evac_center_19_west", 1,
                                            true ) != overmap::invalid_tripoint ) {
        direction = 'W';
    } else {
        return;
    }

    //coords are for evac_center_19_north. will be rotated/reflected for other tile orientations
    int x_coords[18] = {
        1, 1, 1, 2, 2, 2, //first table
        1, 1, 1, 2, 2, 2, //second table
        1, 1, 1, 2, 2, 2 //third table
    };

    int y_coords[18] = {
        5, 6, 7, 5, 6, 7,  //first table
        10, 11, 12, 10, 11, 12, //second table
        15, 16, 17, 15, 16, 17 //third table
    };
    std::string item[18] = {
        "can_beans", "can_spam", "can_tomato", "can_sardine", "soup_veggy", "soup_dumplings", //first table
        "soup_meat", "soup_tomato", "soup_fish", "fish_canned", "curry_veggy", "ravioli",    //second table
        "curry_meat", "chili", "soup_chicken", "pork_beans", "soup_mushroom", "can_tuna"     //third table
    };
    int quantity[18] = {
        3, 2, 3, 2, 3, 2, //first table
        3, 2, 2, 2, 2, 2, //second table
        3, 2, 2, 2, 2, 2 //third table
    };

    // tiles are squares of size 24x24, meaning max index in a direction is 23
    const int max_tile_index = 23;

    for( int i = 0; i < 18; i++ ) {
        switch( direction ) {
            case 'N':
                storageRoom.spawn_item( x_coords[i], y_coords[i], item[i], quantity[i] );
                break;
            case 'E':
                storageRoom.spawn_item( max_tile_index - y_coords[i], x_coords[i], item[i], quantity[i] );
                break;
            case 'S':
                storageRoom.spawn_item( max_tile_index - x_coords[i], max_tile_index - y_coords[i], item[i],
                                        quantity[i] );
                break;
            case 'W':
                storageRoom.spawn_item( y_coords[i], max_tile_index - x_coords[i], item[i], quantity[i] );
                break;

        }
    }
}

