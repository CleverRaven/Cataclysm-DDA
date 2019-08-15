#include "mission.h" // IWYU pragma: associated

#include <vector>
#include <memory>

#include "avatar.h"
#include "computer.h"
#include "debug.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "name.h"
#include "npc.h"
#include "npc_class.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "string_formatter.h"
#include "translations.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "optional.h"
#include "rng.h"

const mtype_id mon_charred_nightmare( "mon_charred_nightmare" );
const mtype_id mon_dog( "mon_dog" );
const mtype_id mon_jabberwock( "mon_jabberwock" );
const mtype_id mon_zombie( "mon_zombie" );
const mtype_id mon_zombie_brute( "mon_zombie_brute" );
const mtype_id mon_zombie_dog( "mon_zombie_dog" );
const mtype_id mon_zombie_electric( "mon_zombie_electric" );
const mtype_id mon_zombie_hulk( "mon_zombie_hulk" );
const mtype_id mon_zombie_master( "mon_zombie_master" );
const mtype_id mon_zombie_necro( "mon_zombie_necro" );

const efftype_id effect_infection( "infection" );

/* These functions are responsible for making changes to the game at the moment
 * the mission is accepted by the player.  They are also responsible for
 * updating *miss with the target and any other important information.
 */

void mission_start::standard( mission * )
{
}

void mission_start::place_dog( mission *miss )
{
    const tripoint house = mission_util::random_house_in_closest_city();
    npc *dev = g->find_npc( miss->npc_id );
    if( dev == nullptr ) {
        debugmsg( "Couldn't find NPC! %d", miss->npc_id );
        return;
    }
    g->u.i_add( item( "dog_whistle", 0 ) );
    add_msg( _( "%s gave you a dog whistle." ), dev->name );

    miss->target = house;
    overmap_buffer.reveal( house, 6 );

    tinymap doghouse;
    doghouse.load( tripoint( house.x * 2, house.y * 2, house.z ), false );
    doghouse.add_spawn( mon_dog, 1, point( SEEX, SEEY ), true, -1, miss->uid );
    doghouse.save();
}

void mission_start::place_zombie_mom( mission *miss )
{
    const tripoint house = mission_util::random_house_in_closest_city();

    miss->target = house;
    overmap_buffer.reveal( house, 6 );

    tinymap zomhouse;
    zomhouse.load( tripoint( house.x * 2, house.y * 2, house.z ), false );
    zomhouse.add_spawn( mon_zombie, 1, point( SEEX, SEEY ), false, -1, miss->uid,
                        Name::get( nameIsFemaleName | nameIsGivenName ) );
    zomhouse.save();
}

void mission_start::kill_horde_master( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->npc_id );
        return;
    }
    // Npc joins you
    p->set_attitude( NPCATT_FOLLOW );
    // Pick one of the below locations for the horde to haunt
    const auto center = p->global_omt_location();
    tripoint site = overmap_buffer.find_closest( center, "office_tower_1", 0, false );
    if( site == overmap::invalid_tripoint ) {
        site = overmap_buffer.find_closest( center, "hotel_tower_1_8", 0, false );
    }
    if( site == overmap::invalid_tripoint ) {
        site = overmap_buffer.find_closest( center, "school_5", 0, false );
    }
    if( site == overmap::invalid_tripoint ) {
        site = overmap_buffer.find_closest( center, "forest_thick", 0, false );
    }
    miss->target = site;
    overmap_buffer.reveal( site, 6 );
    tinymap tile;
    tile.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    tile.add_spawn( mon_zombie_master, 1, point( SEEX, SEEY ), false, -1, miss->uid,
                    _( "Demonic Soul" ) );
    tile.add_spawn( mon_zombie_brute, 3, point( SEEX, SEEY ) );
    tile.add_spawn( mon_zombie_dog, 3, point( SEEX, SEEY ) );

    if( overmap::inbounds( tripoint( SEEX, SEEY, 0 ), 1 ) ) {
        for( int x = SEEX - 1; x <= SEEX + 1; x++ ) {
            for( int y = SEEY - 1; y <= SEEY + 1; y++ ) {
                tile.add_spawn( mon_zombie, rng( 3, 10 ), point( x, y ) );
            }
            tile.add_spawn( mon_zombie_dog, rng( 0, 2 ), point( SEEX, SEEY ) );
        }
    }
    tile.add_spawn( mon_zombie_necro, 2, point( SEEX, SEEY ) );
    tile.add_spawn( mon_zombie_hulk, 1, point( SEEX, SEEY ) );
    tile.save();
}

/*
 * Find a location to place a computer.  In order, prefer:
 * 1) Broken consoles.
 * 2) Corners or coords adjacent to a bed/dresser? (this logic may be flawed, dates from Whales in 2011)
 * 3) A spot near the center of the tile that is not a console
 * 4) A random spot near the center of the tile.
 */
static tripoint find_potential_computer_point( const tinymap &compmap, int z )
{
    constexpr int rng_x_min = 10;
    constexpr int rng_x_max = SEEX * 2 - 11;
    constexpr int rng_y_min = 10;
    constexpr int rng_y_max = SEEY * 2 - 11;
    static_assert( rng_x_min <= rng_x_max && rng_y_min <= rng_y_max, "invalid randomization range" );
    std::vector<tripoint> broken;
    std::vector<tripoint> potential;
    std::vector<tripoint> last_resort;
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            if( compmap.ter( x, y ) == t_console_broken ) {
                broken.emplace_back( x, y, z );
            } else if( broken.empty() && compmap.ter( x, y ) == t_floor && compmap.furn( x, y ) == f_null ) {
                bool okay = false;
                int wall = 0;
                for( int x2 = x - 1; x2 <= x + 1 && !okay; x2++ ) {
                    for( int y2 = y - 1; y2 <= y + 1 && !okay; y2++ ) {
                        if( compmap.furn( x2, y2 ) == f_bed || compmap.furn( x2, y2 ) == f_dresser ) {
                            okay = true;
                            potential.emplace_back( x, y, z );
                        }
                        if( compmap.has_flag_ter( "WALL", x2, y2 ) ) {
                            wall++;
                        }
                    }
                }
                if( wall == 5 ) {
                    if( compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, NORTH ) &&
                        compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, SOUTH ) &&
                        compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, WEST ) &&
                        compmap.is_last_ter_wall( true, x, y, SEEX * 2, SEEY * 2, EAST ) ) {
                        potential.emplace_back( x, y, z );
                    }
                }
            } else if( broken.empty() && potential.empty() && x >= rng_x_min && x <= rng_x_max
                       && y >= rng_y_min && y <= rng_y_max && compmap.ter( x, y ) != t_console ) {
                last_resort.emplace_back( x, y, z );
            }
        }
    }
    std::vector<tripoint> *used = &broken;
    if( used->empty() ) {
        used = &potential;
    }
    if( used->empty() ) {
        used = &last_resort;
    }
    // if there's no possible location, then we have to overwrite an existing console...
    const tripoint fallback( rng( rng_x_min, rng_x_max ), rng( rng_y_min, rng_y_max ), z );
    return random_entry( *used, fallback );
}

void mission_start::place_npc_software( mission *miss )
{
    npc *dev = g->find_npc( miss->npc_id );
    if( dev == nullptr ) {
        debugmsg( "Couldn't find NPC! %d", miss->npc_id );
        return;
    }
    g->u.i_add( item( "usb_drive", 0 ) );
    add_msg( _( "%s gave you a USB drive." ), dev->name );

    std::string type = "house";

    if( dev->myclass == NC_HACKER ) {
        miss->item_id = "software_hacking";
    } else if( dev->myclass == NC_DOCTOR ) {
        miss->item_id = "software_medical";
        type = "s_pharm";
        miss->follow_up = mission_type_id( "MISSION_GET_ZOMBIE_BLOOD_ANAL" );
    } else if( dev->myclass == NC_SCIENTIST ) {
        miss->item_id = "software_math";
    } else {
        miss->item_id = "software_useless";
    }

    tripoint place;
    if( type == "house" ) {
        place = mission_util::random_house_in_closest_city();
    } else {
        place = overmap_buffer.find_closest( dev->global_omt_location(), type, 0, false );
    }
    miss->target = place;
    overmap_buffer.reveal( place, 6 );

    tinymap compmap;
    compmap.load( tripoint( place.x * 2, place.y * 2, place.z ), false );
    tripoint comppoint;

    oter_id oter = overmap_buffer.ter( place );
    if( is_ot_match( "house", oter, ot_match_type::prefix ) ||
        is_ot_match( "s_pharm", oter, ot_match_type::type ) || oter == "" ) {
        comppoint = find_potential_computer_point( compmap, place.z );
    }

    compmap.ter_set( comppoint, t_console );
    computer *tmpcomp = compmap.add_computer( comppoint, string_format( _( "%s's Terminal" ),
                        dev->name ), 0 );
    tmpcomp->mission_id = miss->uid;
    tmpcomp->add_option( _( "Download Software" ), COMPACT_DOWNLOAD_SOFTWARE, 0 );
    compmap.save();
}

void mission_start::place_priest_diary( mission *miss )
{
    const tripoint place = mission_util::random_house_in_closest_city();
    miss->target = place;
    overmap_buffer.reveal( place, 2 );
    tinymap compmap;
    compmap.load( tripoint( place.x * 2, place.y * 2, place.z ), false );

    std::vector<tripoint> valid;
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            if( compmap.furn( x, y ) == f_bed || compmap.furn( x, y ) == f_dresser ||
                compmap.furn( x, y ) == f_indoor_plant || compmap.furn( x, y ) == f_cupboard ) {
                valid.push_back( tripoint( x, y, place.z ) );
            }
        }
    }
    const tripoint fallback( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), place.z );
    const tripoint comppoint = random_entry( valid, fallback );
    compmap.spawn_item( comppoint, "priest_diary" );
    compmap.save();
}

void mission_start::place_deposit_box( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->npc_id );
        return;
    }
    // Npc joins you
    p->set_attitude( NPCATT_FOLLOW );
    tripoint site = overmap_buffer.find_closest( p->global_omt_location(), "bank", 0, false );
    if( site == overmap::invalid_tripoint ) {
        site = overmap_buffer.find_closest( p->global_omt_location(), "office_tower_1", 0, false );
    }

    if( site == overmap::invalid_tripoint ) {
        site = p->global_omt_location();
        debugmsg( "Couldn't find a place for deposit box" );
    }

    miss->target = site;
    overmap_buffer.reveal( site, 2 );

    tinymap compmap;
    compmap.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    std::vector<tripoint> valid;
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            if( compmap.ter( x, y ) == t_floor ) {
                bool okay = false;
                for( int x2 = x - 1; x2 <= x + 1 && !okay; x2++ ) {
                    for( int y2 = y - 1; y2 <= y + 1 && !okay; y2++ ) {
                        if( compmap.ter( x2, y2 ) == t_wall_metal ) {
                            okay = true;
                            valid.push_back( tripoint( x, y, site.z ) );
                        }
                    }
                }
            }
        }
    }
    const tripoint fallback( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), site.z );
    const tripoint comppoint = random_entry( valid, fallback );
    compmap.spawn_item( comppoint, "safe_box" );
    compmap.save();
}

void mission_start::find_safety( mission *miss )
{
    const tripoint place = g->u.global_omt_location();
    for( int radius = 0; radius <= 20; radius++ ) {
        for( int dist = 0 - radius; dist <= radius; dist++ ) {
            int offset = rng( 0, 3 ); // Randomizes the direction we check first
            for( int i = 0; i <= 3; i++ ) { // Which direction?
                tripoint check = place;
                switch( ( offset + i ) % 4 ) {
                    case 0:
                        check.x += dist;
                        check.y -= radius;
                        break;
                    case 1:
                        check.x += dist;
                        check.y += radius;
                        break;
                    case 2:
                        check.y += dist;
                        check.x -= radius;
                        break;
                    case 3:
                        check.y += dist;
                        check.x += radius;
                        break;
                }
                if( overmap_buffer.is_safe( check ) ) {
                    miss->target = check;
                    return;
                }
            }
        }
    }
    // Couldn't find safety; so just set the target to far away
    switch( rng( 0, 3 ) ) {
        case 0:
            miss->target = place + point( -20, -20 );
            break;
        case 1:
            miss->target = place + point( -20, 20 );
            break;
        case 2:
            miss->target = place + point( 20, -20 );
            break;
        case 3:
            miss->target = place + point( 20, 20 );
            break;
    }
}

const int RANCH_SIZE = 5;

void mission_start::ranch_nurse_1( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_furn( f_rack, point( 16, 9 ), point( 17, 9 ) );
    bay.spawn_item( point( 16, 9 ), "bandages", rng( 1, 3 ) );
    bay.spawn_item( point( 17, 9 ), "aspirin", rng( 1, 2 ) );
    bay.save();
}

void mission_start::ranch_nurse_2( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_furn( f_counter, point( 3, 7 ), point( 5, 7 ) );
    bay.draw_square_furn( f_rack, point( 8, 4 ), point( 8, 5 ) );
    bay.spawn_item( point( 8, 4 ), "manual_first_aid" );
    bay.save();
}

void mission_start::ranch_nurse_3( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_ter( t_dirt, point( 2, 16 ), point( 9, 23 ) );
    bay.draw_square_ter( t_dirt, point( 13, 16 ), point( 20, 23 ) );
    bay.draw_square_ter( t_dirt, point( 10, 17 ), point( 12, 23 ) );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_ter( t_dirt, point( 2, 0 ), point( 20, 2 ) );
    bay.draw_square_ter( t_dirt, point( 10, 3 ), point( 12, 4 ) );
    bay.save();
}

void mission_start::ranch_nurse_4( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_ter( t_wall_half, point( 2, 16 ), point( 9, 23 ) );
    bay.draw_square_ter( t_dirt, point( 3, 17 ), point( 8, 22 ) );
    bay.draw_square_ter( t_wall_half, point( 13, 16 ), point( 20, 23 ) );
    bay.draw_square_ter( t_dirt, point( 14, 17 ), point( 19, 22 ) );
    bay.draw_square_ter( t_wall_half, point( 10, 17 ), point( 12, 23 ) );
    bay.draw_square_ter( t_dirt, point( 10, 18 ), point( 12, 23 ) );
    bay.ter_set( point( 9, 19 ), t_door_frame );
    bay.ter_set( point( 13, 19 ), t_door_frame );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_ter( t_wall_half, point( 4, 0 ), point( 18, 2 ) );
    bay.draw_square_ter( t_wall_half, point( 10, 3 ), point( 12, 4 ) );
    bay.draw_square_ter( t_dirt, point( 5, 0 ), point( 8, 2 ) );
    bay.draw_square_ter( t_dirt, point( 10, 0 ), point( 12, 4 ) );
    bay.draw_square_ter( t_dirt, point( 14, 0 ), point( 17, 2 ) );
    bay.ter_set( point( 9, 1 ), t_door_frame );
    bay.ter_set( point( 13, 1 ), t_door_frame );
    bay.save();
}

void mission_start::ranch_nurse_5( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.ter_set( point( 2, 21 ), t_window_frame );
    bay.ter_set( point( 2, 18 ), t_window_frame );
    bay.ter_set( point( 20, 18 ), t_window_frame );
    bay.ter_set( point( 20, 21 ), t_window_frame );
    bay.ter_set( point( 11, 17 ), t_window_frame );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.draw_square_ter( t_dirt, point( 10, 0 ), point( 12, 4 ) );
    bay.save();
}

void mission_start::ranch_nurse_6( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_window_frame, t_window_boarded_noglass );
    bay.translate( t_door_frame, t_door_c );
    bay.draw_square_ter( t_dirtfloor, point( 3, 17 ), point( 8, 22 ) );
    bay.draw_square_ter( t_dirtfloor, point( 14, 17 ), point( 19, 22 ) );
    bay.draw_square_ter( t_dirtfloor, point( 10, 18 ), point( 12, 23 ) );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_door_frame, t_door_c );
    bay.draw_square_ter( t_dirtfloor, point( 5, 0 ), point( 8, 2 ) );
    bay.draw_square_ter( t_dirtfloor, point( 10, 0 ), point( 12, 4 ) );
    bay.draw_square_ter( t_dirtfloor, point( 14, 0 ), point( 17, 2 ) );
    bay.save();
}

void mission_start::ranch_nurse_7( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_dirtfloor, t_floor );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_dirtfloor, t_floor );
    bay.draw_square_ter( t_floor, point( 10, 5 ), point( 12, 5 ) );
    bay.draw_square_furn( f_rack, point( 17, 0 ), point( 17, 2 ) );
    bay.save();
}

void mission_start::ranch_nurse_8( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_furn( f_makeshift_bed, point( 4, 21 ), point( 4, 22 ) );
    bay.draw_square_furn( f_makeshift_bed, point( 7, 21 ), point( 7, 22 ) );
    bay.draw_square_furn( f_makeshift_bed, point( 15, 21 ), point( 15, 22 ) );
    bay.draw_square_furn( f_makeshift_bed, point( 18, 21 ), point( 18, 22 ) );
    bay.draw_square_furn( f_makeshift_bed, point( 4, 17 ), point( 4, 18 ) );
    bay.draw_square_furn( f_makeshift_bed, point( 7, 17 ), point( 7, 18 ) );
    bay.draw_square_furn( f_makeshift_bed, point( 15, 17 ), point( 15, 18 ) );
    bay.draw_square_furn( f_makeshift_bed, point( 18, 17 ), point( 18, 18 ) );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_dirtfloor, t_floor );
    bay.place_items( "cleaning", 75, point( 17, 0 ), point( 17, 2 ), true, 0 );
    bay.place_items( "surgery", 75, point( 15, 4 ), point( 18, 4 ), true, 0 );
    bay.save();
}

void mission_start::ranch_nurse_9( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.furn_set( point( 3, 22 ), f_dresser );
    bay.furn_set( point( 8, 22 ), f_dresser );
    bay.furn_set( point( 14, 22 ), f_dresser );
    bay.furn_set( point( 19, 22 ), f_dresser );
    bay.furn_set( point( 3, 17 ), f_dresser );
    bay.furn_set( point( 8, 17 ), f_dresser );
    bay.furn_set( point( 14, 17 ), f_dresser );
    bay.furn_set( point( 19, 17 ), f_dresser );
    bay.place_npc( point( 16, 19 ), string_id<npc_template>( "ranch_doctor" ) );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_scavenger_1( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_48", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.draw_square_ter( t_chainfence, point( 15, 13 ), point( 15, 22 ) );
    bay.draw_square_ter( t_chainfence, point( 16, 13 ), point( 23, 13 ) );
    bay.draw_square_ter( t_chainfence, point( 16, 22 ), point( 23, 22 ) );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.place_items( "mechanics", 65, point( 9, 13 ), point( 10, 16 ), true, 0 );
    bay.draw_square_ter( t_chainfence, point( 0, 22 ), point( 7, 22 ) );
    bay.draw_square_ter( t_dirt, point( 2, 22 ), point( 3, 22 ) );
    bay.spawn_item( point( 7, 19 ), "30gal_drum" );
    bay.save();
}

void mission_start::ranch_scavenger_2( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_48", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.add_vehicle( vproto_id( "car_chassis" ), 20, 15, 0 );
    bay.draw_square_ter( t_wall_half, point( 18, 19 ), point( 21, 22 ) );
    bay.draw_square_ter( t_dirt, point( 19, 20 ), point( 20, 21 ) );
    bay.ter_set( point( 19, 19 ), t_door_frame );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.place_items( "mischw", 65, point( 12, 13 ), point( 13, 16 ), true, 0 );
    bay.draw_square_ter( t_chaingate_l, point( 2, 22 ), point( 3, 22 ) );
    bay.spawn_item( point( 7, 20 ), "30gal_drum" );
    bay.save();
}

void mission_start::ranch_scavenger_3( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_48", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.translate( t_door_frame, t_door_locked );
    bay.translate( t_wall_half, t_wall_wood );
    bay.draw_square_ter( t_dirtfloor, point( 19, 20 ), point( 20, 21 ) );
    bay.spawn_item( point( 16, 21 ), "wheel_wide" );
    bay.spawn_item( point( 17, 21 ), "wheel_wide" );
    bay.spawn_item( point( 23, 18 ), "v8_combustion" );
    bay.furn_set( point( 23, 17 ), furn_str_id( "f_arcade_machine" ) );
    bay.ter_set( point( 23, 16 ), ter_str_id( "t_machinery_light" ) );
    bay.furn_set( point( 20, 21 ), f_woodstove );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    bay.load( tripoint( site.x * 2, site.y * 2, site.z ), false );
    bay.place_items( "mischw", 65, point( 2, 10 ), point( 4, 10 ), true, 0 );
    bay.place_items( "mischw", 65, point( 2, 13 ), point( 4, 13 ), true, 0 );
    bay.furn_set( point( 1, 15 ), f_fridge );
    bay.spawn_item( point( 2, 15 ), "hdframe" );
    bay.furn_set( point( 3, 15 ), f_washer );
    bay.save();
}

void mission_start::place_book( mission * )
{
}

void mission_start::reveal_refugee_center( mission *miss )
{
    mission_target_params t;
    t.overmap_terrain = "refctr_S3e";
    t.overmap_special = overmap_special_id( "evac_center" );
    t.mission_pointer = miss;
    t.search_range = 0;
    t.reveal_radius = 3;

    const cata::optional<tripoint> target_pos = mission_util::assign_mission_target( t );

    if( !target_pos ) {
        add_msg( _( "You don't know where the address could be..." ) );
        return;
    }

    const tripoint source_road = overmap_buffer.find_closest( g->u.global_omt_location(), "road",
                                 3, false );
    const tripoint dest_road = overmap_buffer.find_closest( *target_pos, "road", 3, false );

    if( overmap_buffer.reveal_route( source_road, dest_road, 1, true ) ) {
        add_msg( _( "You mark the refugee center and the road that leads to it..." ) );
    } else {
        add_msg( _( "You mark the refugee center, but you have no idea how to get there by road..." ) );
    }
}

// Creates multiple lab consoles near tripoint place, which must have its z-level set to where consoles should go.
void static create_lab_consoles( mission *miss, const tripoint &place, const std::string &otype,
                                 int security,
                                 const std::string &comp_name, const std::string &download_name )
{
    // Drop four computers in nearby lab spaces so the player can stumble upon one of them.
    for( int i = 0; i < 4; ++i ) {
        tripoint om_place = mission_util::target_om_ter_random( otype, -1, miss, false, 4, place );

        tinymap compmap;
        compmap.load( tripoint( om_place.x * 2, om_place.y * 2, om_place.z ), false );

        tripoint comppoint = find_potential_computer_point( compmap, om_place.z );

        computer *tmpcomp = compmap.add_computer( comppoint, _( comp_name ), security );
        tmpcomp->mission_id = miss->get_id();
        tmpcomp->add_option( _( download_name ), COMPACT_DOWNLOAD_SOFTWARE, security );
        tmpcomp->add_failure( COMPFAIL_ALARM );
        tmpcomp->add_failure( COMPFAIL_DAMAGE );
        tmpcomp->add_failure( COMPFAIL_MANHACKS );

        compmap.save();
    }
}

void mission_start::create_lab_console( mission *miss )
{
    // Pick a lab that has spaces on z = -1: e.g., in hidden labs.
    tripoint loc = g->u.global_omt_location();
    loc.z = -1;
    const tripoint place = overmap_buffer.find_closest( loc, "lab", 0, false );

    create_lab_consoles( miss, place, "lab", 2, translate_marker( "Workstation" ),
                         translate_marker( "Download Memory Contents" ) );

    // Target the lab entrance.
    const tripoint target = mission_util::target_closest_lab_entrance( place, 2, miss );
    mission_util::reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}

void mission_start::create_hidden_lab_console( mission *miss )
{
    // Pick a hidden lab entrance.
    tripoint loc = g->u.global_omt_location();
    loc.z = -1;
    tripoint place = mission_util::target_om_ter_random( "basement_hidden_lab_stairs", -1, miss, false,
                     0, loc );
    place.z = -2;  // then go down 1 z-level to place consoles.

    create_lab_consoles( miss, place, "lab", 3, translate_marker( "Workstation" ),
                         translate_marker( "Download Encryption Routines" ) );

    // Target the lab entrance.
    const tripoint target = mission_util::target_closest_lab_entrance( place, 2, miss );
    mission_util::reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}

void mission_start::create_ice_lab_console( mission *miss )
{
    // Pick an ice lab with spaces on z = -4.
    tripoint loc = g->u.global_omt_location();
    loc.z = -4;
    const tripoint place = overmap_buffer.find_closest( loc, "ice_lab", 0, false );

    create_lab_consoles( miss, place, "ice_lab", 3, translate_marker( "Durable Storage Archive" ),
                         translate_marker( "Download Archives" ) );

    // Target the lab entrance.
    const tripoint target = mission_util::target_closest_lab_entrance( place, 2, miss );
    mission_util::reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}

void mission_start::reveal_lab_train_depot( mission *miss )
{
    // Find and prepare lab location.
    tripoint loc = g->u.global_omt_location();
    loc.z = -4;  // tunnels are at z = -4
    const tripoint place = overmap_buffer.find_closest( loc, "lab_train_depot", 0, false );

    tinymap compmap;
    compmap.load( tripoint( place.x * 2, place.y * 2, place.z ), false );
    tripoint comppoint;

    for( tripoint point : compmap.points_in_rectangle(
             tripoint( 0, 0, place.z ), tripoint( SEEX * 2 - 1, SEEY * 2 - 1, place.z ) ) ) {
        if( compmap.ter( point ) == t_console ) {
            comppoint = point;
            break;
        }
    }

    if( comppoint == tripoint() ) {
        debugmsg( "Could not find a computer in the lab train depot, mission will fail." );
        return;
    }

    computer *tmpcomp = compmap.computer_at( comppoint );
    tmpcomp->mission_id = miss->uid;
    tmpcomp->add_option( _( "Download Routing Software" ), COMPACT_DOWNLOAD_SOFTWARE, 0 );

    compmap.save();

    // Target the lab entrance.
    const tripoint target = mission_util::target_closest_lab_entrance( place, 2, miss );
    mission_util::reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}
