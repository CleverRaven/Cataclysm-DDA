#include "mission.h"

#include "coordinate_conversions.h"
#include "game.h"
#include "map.h"
#include "output.h"
#include "debug.h"
#include "name.h"
#include <sstream>
#include "string_formatter.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "mapdata.h"
#include "messages.h"
#include "translations.h"
#include "overmap.h"
#include "trap.h"
#include "line.h"
#include "computer.h"
// TODO: Remove this include once 2D wrappers are no longer needed
#include "mapgen_functions.h"
#include "field.h"
#include "npc.h"
#include "npc_class.h"

const mtype_id mon_charred_nightmare( "mon_charred_nightmare" );
const mtype_id mon_dog( "mon_dog" );
const mtype_id mon_graboid( "mon_graboid" );
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

/**
 * Given a (valid!) city reference, select a random house within the city borders.
 * @return global overmap terrain coordinates of the house.
 */
static tripoint random_house_in_city( const city_reference &cref )
{
    const auto city_center_omt = sm_to_omt_copy( cref.abs_sm_pos );
    const auto size = cref.city->s;
    const int z = cref.abs_sm_pos.z;
    std::vector<tripoint> valid;
    int startx = city_center_omt.x - size;
    int endx   = city_center_omt.x + size;
    int starty = city_center_omt.y - size;
    int endy   = city_center_omt.y + size;
    for( int x = startx; x <= endx; x++ ) {
        for( int y = starty; y <= endy; y++ ) {
            if( overmap_buffer.check_ot_type( "house", x, y, z ) ) {
                valid.push_back( tripoint( x, y, z ) );
            }
        }
    }
    return random_entry( valid, city_center_omt ); // center of the city is a good fallback
}

static tripoint random_house_in_closest_city()
{
    const auto center = g->u.global_sm_location();
    const auto cref = overmap_buffer.closest_city( center );
    if( !cref ) {
        debugmsg( "could not find closest city" );
        return g->u.global_omt_location();
    }
    return random_house_in_city( cref );
}
/**
 * Set target of mission to closest overmap terrain of that type,
 * reveal the area around it (uses reveal with reveal_rad),
 * and returns the mission target.
 */
static tripoint target_om_ter( const std::string &omter, int reveal_rad, mission *miss,
                               bool must_see )
{
    // The missions are coded to work on z-level 0, so we have to check for locations there
    tripoint surface_loc = g->u.global_omt_location();
    surface_loc.z = 0;
    const tripoint place = overmap_buffer.find_closest( surface_loc, omter, 0, must_see );
    if( place != overmap::invalid_tripoint && reveal_rad >= 0 ) {
        overmap_buffer.reveal( place, reveal_rad );
    }
    miss->set_target( place );
    return place;
}

static tripoint target_om_ter_random( const std::string &omter, int reveal_rad, mission *miss,
                               bool must_see, int range )
{
    auto places = overmap_buffer.find_all( g->u.global_omt_location(), omter, range, must_see );
    if( places.empty() ) {
        return g->u.global_omt_location();
    }
    const auto &cur_om = g->get_cur_om();
    std::vector<tripoint> places_om;
    for( auto &i : places ) {
        if( &cur_om == overmap_buffer.get_existing_om_global( i ) ) {
            places_om.push_back( i );
        }
    }

    const tripoint place = random_entry( places_om );
    if( reveal_rad >= 0 ) {
        overmap_buffer.reveal( place, reveal_rad );
    }
    miss->set_target( place );
    return place;
}

/**
 * Wraps target_om_ter_random() and takes an extra argument specifying the
 * type of tile it is permitted to replace. If it doesn't find an extant tile
 * of the specified type (e.g. bandit_camp_1) it will find a random unexplored
 * tile of the fallback type and replace it with the original type. If even
 * that fails, it will print a debug message.
 */
static tripoint target_om_ter_random_or_create( const std::string &omter, int reveal_rad,
        mission *miss, bool must_see, int range, const std::string &replace_omter )
{
    tripoint site = target_om_ter_random(omter, reveal_rad, miss, must_see, range);

    // If no suitable site is found nearby, make one in an unvisited tile of type `replace_omter`
    if (site == g->u.global_omt_location()) {
        for ( int tries = 10 * range; tries > 0; --tries ) {
            site = target_om_ter_random( replace_omter, 1, miss, false, range  );
            if ( !overmap_buffer.is_explored( site.x, site.y, site.z ) ) {
                overmap_buffer.ter( site ) = oter_id( omter );
                miss->set_target( site );
                return site;
            }
        }
        debugmsg( "Failed to find either an extant overmap tile of type %s, or an unvisited tile "
                  "of type %s that could be replaced with one. (Search radius: %d)",
                  omter, replace_omter, range );
    }

    return site;
}

void mission_start::standard( mission * )
{
}

void mission_start::join( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->set_attitude( NPCATT_FOLLOW );
}

void mission_start::infect_npc( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    if( p == NULL ) {
        debugmsg( "mission_start::infect_npc() couldn't find an NPC!" );
        return;
    }
    p->add_effect( effect_infection, 1_turns, num_bp, 1, true );
    // make sure they don't have any antibiotics
    p->remove_items_with( []( const item & it ) {
        return it.typeId() == "antibiotics";
    } );
    // Make sure they stay here
    p->guard_current_pos();
}

void mission_start::need_drugs_npc( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    if( p == NULL ) {
        debugmsg( "mission_start::need_drugs_npc() couldn't find an NPC!" );
        return;
    }
    // make sure they don't have any item goal
    p->remove_items_with( [&]( const item & it ) {
        return it.typeId() == miss->item_id;
    } );
    // Make sure they stay here
    p->guard_current_pos();
}

void mission_start::place_dog( mission *miss )
{
    const tripoint house = random_house_in_closest_city();
    npc *dev = g->find_npc( miss->npc_id );
    if( dev == NULL ) {
        debugmsg( "Couldn't find NPC! %d", miss->npc_id );
        return;
    }
    g->u.i_add( item( "dog_whistle", 0 ) );
    add_msg( _( "%s gave you a dog whistle." ), dev->name.c_str() );

    miss->target = house;
    overmap_buffer.reveal( house, 6 );

    tinymap doghouse;
    doghouse.load( house.x * 2, house.y * 2, house.z, false );
    doghouse.add_spawn( mon_dog, 1, SEEX, SEEY, true, -1, miss->uid );
    doghouse.save();
}

void mission_start::place_zombie_mom( mission *miss )
{
    const tripoint house = random_house_in_closest_city();

    miss->target = house;
    overmap_buffer.reveal( house, 6 );

    tinymap zomhouse;
    zomhouse.load( house.x * 2, house.y * 2, house.z, false );
    zomhouse.add_spawn( mon_zombie, 1, SEEX, SEEY, false, -1, miss->uid,
                        Name::get( nameIsFemaleName | nameIsGivenName ) );
    zomhouse.save();
}

const int EVAC_CENTER_SIZE = 5;

void mission_start::place_zombie_bay( mission *miss )
{
    tripoint site = target_om_ter_random( "evac_center_9", 1, miss, false, EVAC_CENTER_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.add_spawn( mon_zombie_electric, 1, SEEX, SEEY, false, -1, miss->uid, _( "Sean McLaughlin" ) );
    bay.save();
}

void mission_start::place_caravan_ambush( mission *miss )
{
    tripoint site = target_om_ter_random( "field", 1, miss, false, 80 );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.add_vehicle( vproto_id( "cube_van" ), SEEX, SEEY, 0 );
    bay.add_vehicle( vproto_id( "quad_bike" ), SEEX + 6, SEEY - 5, 270, 500, -1, true );
    bay.add_vehicle( vproto_id( "motorcycle" ), SEEX - 2, SEEY - 9, 315, 500, -1, true );
    bay.add_vehicle( vproto_id( "motorcycle" ), SEEX - 5, SEEY - 5, 90, 500, -1, true );
    bay.draw_square_ter( t_grass, SEEX - 6, SEEY - 9, SEEX + 6, SEEY + 3 );
    bay.draw_square_ter( t_dirt, SEEX - 4, SEEY - 7, SEEX + 3, SEEY + 1 );
    bay.furn_set( SEEX, SEEY - 4, f_ash );
    bay.spawn_item( SEEX - 1, SEEY - 3, "rock" );
    bay.spawn_item( SEEX, SEEY - 3, "rock" );
    bay.spawn_item( SEEX + 1, SEEY - 3, "rock" );
    bay.spawn_item( SEEX - 1, SEEY - 4, "rock" );
    bay.spawn_item( SEEX + 1, SEEY - 4, "rock" );
    bay.spawn_item( SEEX - 1, SEEY - 5, "rock" );
    bay.spawn_item( SEEX, SEEY - 5, "rock" );
    bay.spawn_item( SEEX + 1, SEEY - 5, "rock" );
    bay.trap_set( {SEEX + 3, SEEY - 5, site.z}, tr_rollmat );
    bay.trap_set( {SEEX, SEEY - 7, site.z}, tr_rollmat );
    bay.trap_set( {SEEX - 3, SEEY - 4, site.z}, tr_fur_rollmat );
    bay.spawn_item( SEEX + rng( -6, 6 ), SEEY + rng( -9, 3 ), "can_beans" );
    bay.spawn_item( SEEX + rng( -6, 6 ), SEEY + rng( -9, 3 ), "beer" );
    bay.spawn_item( SEEX + rng( -6, 6 ), SEEY + rng( -9, 3 ), "beer" );
    bay.spawn_item( SEEX + rng( -6, 6 ), SEEY + rng( -9, 3 ), "bottle_glass" );
    bay.spawn_item( SEEX + rng( -6, 6 ), SEEY + rng( -9, 3 ), "bottle_glass" );
    bay.spawn_item( SEEX + rng( -6, 6 ), SEEY + rng( -9, 3 ), "heroin" );
    bay.place_items( "dresser", 75, SEEX - 3, SEEY, SEEX - 2, SEEY + 2, true, 0 );
    bay.place_items( "softdrugs", 50, SEEX - 3, SEEY, SEEX - 2, SEEY + 2, true, 0 );
    bay.place_items( "camping", 75, SEEX - 3, SEEY, SEEX - 2, SEEY + 2, true, 0 );
    bay.spawn_item( SEEX + 1, SEEY + 4, "9mm_casing", 1, 1, 0, 0 );
    bay.spawn_item( SEEX + rng( -2, 3 ), SEEY + rng( 3, 6 ), "9mm_casing", 1, 1, 0, 0 );
    bay.spawn_item( SEEX + rng( -2, 3 ), SEEY + rng( 3, 6 ), "9mm_casing", 1, 1, 0, 0 );
    bay.spawn_item( SEEX + rng( -2, 3 ), SEEY + rng( 3, 6 ), "9mm_casing", 1, 1, 0, 0 );
    bay.add_corpse( tripoint( SEEX + 1, SEEY + 7, bay.get_abs_sub().z ) );
    bay.add_corpse( tripoint( SEEX, SEEY + 8, bay.get_abs_sub().z ) );
    madd_field( &bay, SEEX, SEEY + 7, fd_blood, 1 );
    madd_field( &bay, SEEX + 2, SEEY + 7, fd_blood, 1 );
    madd_field( &bay, SEEX - 1, SEEY + 8, fd_blood, 1 );
    madd_field( &bay, SEEX + 1, SEEY + 8, fd_blood, 1 );
    madd_field( &bay, SEEX + 2, SEEY + 8, fd_blood, 1 );
    madd_field( &bay, SEEX + 1, SEEY + 9, fd_blood, 1 );
    madd_field( &bay, SEEX, SEEY + 9, fd_blood, 1 );
    bay.place_npc( SEEX + 3, SEEY - 5, string_id<npc_template>( "bandit" ) );
    bay.place_npc( SEEX, SEEY - 7, string_id<npc_template>( "thug" ) );
    miss->target_npc_id = bay.place_npc( SEEX - 3, SEEY - 4, string_id<npc_template>( "bandit" ) );
    bay.save();
}

void mission_start::place_bandit_cabin( mission *miss )
{
    tripoint site = target_om_ter_random_or_create( "bandit_cabin", 1, miss, false, 50, "forest" );

    tinymap cabin;
    cabin.load( site.x * 2, site.y * 2, site.z, false );
    cabin.trap_set( {SEEX - 5, SEEY - 6, site.z}, tr_landmine_buried );
    cabin.trap_set( {SEEX - 7, SEEY - 7, site.z}, tr_landmine_buried );
    cabin.trap_set( {SEEX - 4, SEEY - 7, site.z}, tr_landmine_buried );
    cabin.trap_set( {SEEX - 12, SEEY - 1, site.z}, tr_landmine_buried );
    miss->target_npc_id = cabin.place_npc( SEEX, SEEY, string_id<npc_template>( "bandit" ) );
    cabin.save();
}

void mission_start::place_informant( mission *miss )
{
    tripoint site = target_om_ter_random( "evac_center_19", 1, miss, false, EVAC_CENTER_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    miss->target_npc_id = bay.place_npc( SEEX, SEEY, string_id<npc_template>( "evac_guard3" ) );
    bay.save();

    site = target_om_ter_random( "evac_center_7", 1, miss, false, EVAC_CENTER_SIZE );
    tinymap bay2;
    bay2.load( site.x * 2, site.y * 2, site.z, false );
    bay2.place_npc( SEEX + rng( -3, 3 ), SEEY + rng( -3, 3 ), string_id<npc_template>( "scavenger_hunter" ) );
    bay2.save();
    target_om_ter_random( "evac_center_17", 1, miss, false, EVAC_CENTER_SIZE );
}

void mission_start::place_grabber( mission *miss )
{
    tripoint site = target_om_ter_random( "field", 5, miss, false, 50 );
    tinymap there;
    there.load( site.x * 2, site.y * 2, site.z, false );
    there.add_spawn( mon_graboid, 1, SEEX + rng( -3, 3 ), SEEY + rng( -3, 3 ) );
    there.add_spawn( mon_graboid, 1, SEEX, SEEY, false, -1, miss->uid, _( "Little Guy" ) );
    there.save();
}

void mission_start::place_bandit_camp( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    g->u.i_add( item( "ruger_redhawk", calendar::turn ) );
    g->u.i_add( item( "44magnum", calendar::turn ) );
    g->u.i_add( item( "holster", calendar::turn ) );
    g->u.i_add( item( "badge_marshal", calendar::turn ) );
    add_msg( m_good, _( "%s has instated you as a marshal!" ), p->name.c_str() );
    // Ideally this would happen at the end of the mission
    // (you're told that they entered your image into the databases, etc)
    // but better to get it working.
    g->u.set_mutation( trait_id( "PROF_FED" ) );

    tripoint site = target_om_ter_random_or_create( "bandit_camp_1", 1, miss, false, 50, "forest" );

    tinymap bay1;
    bay1.load( site.x * 2, site.y * 2, site.z, false );
    miss->target_npc_id = bay1.place_npc( SEEX + 5, SEEY - 3, string_id<npc_template>( "bandit" ) );
    bay1.save();
}

void mission_start::place_jabberwock( mission *miss )
{
    tripoint site = target_om_ter( "forest_thick", 6, miss, false );
    tinymap grove;
    grove.load( site.x * 2, site.y * 2, site.z, false );
    grove.add_spawn( mon_jabberwock, 1, SEEX, SEEY, false, -1, miss->uid, "NONE" );
    grove.save();
}

void mission_start::kill_100_z( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->set_attitude( NPCATT_FOLLOW );//npc joins you
    //kill count of the monsters from a given species you need to reach
    miss->kill_count_to_reach = g->kill_count( miss->monster_species ) + miss->monster_kill_goal;
}

void mission_start::kill_20_nightmares( mission *miss )
{
    target_om_ter( "necropolis_c_44", 3, miss, false );
    miss->monster_type = mon_charred_nightmare.str();
    //kill count of the monster type you need to reach
    miss->kill_count_to_reach = g->kill_count( mon_charred_nightmare ) + miss->monster_kill_goal;
}

void mission_start::kill_horde_master( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->set_attitude( NPCATT_FOLLOW );//npc joins you
    //pick one of the below locations for the horde to haunt
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
    tile.load( site.x * 2, site.y * 2, site.z, false );
    tile.add_spawn( mon_zombie_master, 1, SEEX, SEEY, false, -1, miss->uid, _( "Demonic Soul" ) );
    tile.add_spawn( mon_zombie_brute, 3, SEEX, SEEY );
    tile.add_spawn( mon_zombie_dog, 3, SEEX, SEEY );

    if( overmap::inbounds( SEEX, SEEY, 0, 1 ) ) {
        for( int x = SEEX - 1; x <= SEEX + 1; x++ ) {
            for( int y = SEEY - 1; y <= SEEY + 1; y++ ) {
                tile.add_spawn( mon_zombie, rng( 3, 10 ), x, y );
            }
            tile.add_spawn( mon_zombie_dog, rng( 0, 2 ), SEEX, SEEY );
        }
    }
    tile.add_spawn( mon_zombie_necro, 2, SEEX, SEEY );
    tile.add_spawn( mon_zombie_hulk, 1, SEEX, SEEY );
    tile.save();
}

void mission_start::place_npc_software( mission *miss )
{
    npc *dev = g->find_npc( miss->npc_id );
    if( dev == NULL ) {
        debugmsg( "Couldn't find NPC! %d", miss->npc_id );
        return;
    }
    g->u.i_add( item( "usb_drive", 0 ) );
    add_msg( _( "%s gave you a USB drive." ), dev->name.c_str() );

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
        place = random_house_in_closest_city();
    } else {
        place = overmap_buffer.find_closest( dev->global_omt_location(), type, 0, false );
    }
    miss->target = place;
    overmap_buffer.reveal( place, 6 );

    tinymap compmap;
    compmap.load( place.x * 2, place.y * 2, place.z, false );
    tripoint comppoint;

    oter_id oter = overmap_buffer.ter( place.x, place.y, place.z );
    if( is_ot_type( "house", oter ) || is_ot_type( "s_pharm", oter ) || oter == "" ) {
        std::vector<tripoint> valid;
        for( int x = 0; x < SEEX * 2; x++ ) {
            for( int y = 0; y < SEEY * 2; y++ ) {
                if( compmap.ter( x, y ) == t_floor && compmap.furn( x, y ) == f_null ) {
                    bool okay = false;
                    int wall = 0;
                    for( int x2 = x - 1; x2 <= x + 1 && !okay; x2++ ) {
                        for( int y2 = y - 1; y2 <= y + 1 && !okay; y2++ ) {
                            if( compmap.furn( x2, y2 ) == f_bed || compmap.furn( x2, y2 ) == f_dresser ) {
                                okay = true;
                                valid.push_back( tripoint( x, y, place.z ) );
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
                            valid.push_back( tripoint( x, y, place.z ) );
                        }
                    }
                }
            }
        }
        const tripoint fallback( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), place.z );
        comppoint = random_entry( valid, fallback );
    }

    compmap.ter_set( comppoint, t_console );
    computer *tmpcomp = compmap.add_computer( comppoint, string_format( _( "%s's Terminal" ), dev->name.c_str() ), 0 );
    tmpcomp->mission_id = miss->uid;
    tmpcomp->add_option( _( "Download Software" ), COMPACT_DOWNLOAD_SOFTWARE, 0 );
    compmap.save();
}

void mission_start::place_priest_diary( mission *miss )
{
    const tripoint place = random_house_in_closest_city();
    miss->target = place;
    overmap_buffer.reveal( place, 2 );
    tinymap compmap;
    compmap.load( place.x * 2, place.y * 2, place.z, false );

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
    p->set_attitude( NPCATT_FOLLOW );//npc joins you
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
    compmap.load( site.x * 2, site.y * 2, site.z, false );
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

void mission_start::reveal_lab_black_box( mission *miss )
{
    npc *dev = g->find_npc( miss->npc_id );
    if( dev != NULL ) {
        g->u.i_add( item( "black_box", 0 ) );
        add_msg( _( "%s gave you back the black box." ), dev->name.c_str() );
    }
    target_om_ter( "lab", 3, miss, false );
}

void mission_start::open_sarcophagus( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    if( p != NULL ) {
        p->set_attitude( NPCATT_FOLLOW );
        g->u.i_add( item( "sarcophagus_access_code", 0 ) );
        add_msg( m_good, _( "%s gave you sarcophagus access code." ), p->name.c_str() );
    } else {
        DebugLog( D_ERROR, DC_ALL ) << "mission_start: open_sarcophagus() <= Can't find NPC";
    }
    target_om_ter( "haz_sar", 3, miss, false );
}

void mission_start::reveal_hospital( mission *miss )
{
    npc *dev = g->find_npc( miss->npc_id );
    if( dev != NULL ) {
        g->u.i_add( item( "vacutainer", 0 ) );
        add_msg( _( "%s gave you a blood draw kit." ), dev->name.c_str() );
    }
    target_om_ter( "hospital_2", 3, miss, false );
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
            miss->target = tripoint( place.x - 20, place.y - 20, place.z );
            break;
        case 1:
            miss->target = tripoint( place.x - 20, place.y + 20, place.z );
            break;
        case 2:
            miss->target = tripoint( place.x + 20, place.y - 20, place.z );
            break;
        case 3:
            miss->target = tripoint( place.x + 20, place.y + 20, place.z );
            break;
    }
}

void mission_start::point_prison( mission *miss )
{
    target_om_ter( "prison_1_5", 3, miss, false );
}

void mission_start::point_cabin_strange( mission *miss )
{
    target_om_ter( "cabin_strange", 3, miss, false );
}

void mission_start::recruit_tracker( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->set_attitude( NPCATT_FOLLOW );// NPC joins you

    tripoint site = target_om_ter( "cabin", 2, miss, false );
    miss->recruit_class = NC_COWBOY;

    std::shared_ptr<npc> temp = std::make_shared<npc>();
    temp->normalize();
    temp->randomize( NC_COWBOY );
    // NPCs spawn with submap coordinates, site is in overmap terrain coordinates
    temp->spawn_at_precise( { site.x * 2, site.y * 2 }, tripoint( 11, 11, site.z ) );
    overmap_buffer.insert_npc( temp );
    temp->set_attitude( NPCATT_TALK );
    temp->mission = NPC_MISSION_SHOPKEEP;
    temp->personality.aggression -= 1;
    temp->op_of_u.owed = 10;
    temp->add_new_mission( mission::reserve_new( mission_type_id( "MISSION_JOIN_TRACKER" ), temp->getID() ) );
}

void mission_start::radio_repeater( mission *miss )
{
    target_om_ter( "necropolis_c_23", 3, miss, false );
    g->u.i_add( item( "repeater_mod_guide", calendar::turn ) );
}

void mission_start::start_commune(mission *miss)
{
 // Check entire overmap for now.
 tripoint site = target_om_ter("ranch_camp_67",1,miss,false);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_npc(SEEX+4, SEEY+3, string_id<npc_template>( "ranch_foreman" ) );
 bay.place_npc(SEEX-3, SEEY+5, string_id<npc_template>( "ranch_construction_1" ) );
 bay.save();
 npc *p = g->find_npc( miss->npc_id );
 p->set_mutation( trait_id( "NPC_MISSION_LEV_1" ) );
}

const int RANCH_SIZE = 5;

void mission_start::ranch_construct_1(mission *miss)
{
 tripoint site = target_om_ter_random("ranch_camp_66", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.ter_set(1, 0, t_wall_wood);
 bay.ter_set(0, 0, t_door_c);
 bay.ter_set(13, 0, t_wall_wood);
 bay.ter_set(14, 0, t_door_c);
 bay.ter_set(16, 0, t_wall_wood);
 bay.ter_set(15, 0, t_door_c);
 bay.save();
 site = target_om_ter_random("ranch_camp_65", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.ter_set(22, 0, t_wall_wood);
 bay.ter_set(23, 0, t_door_c);
 bay.save();
 site = target_om_ter_random("ranch_camp_74", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.ter_set(22, 0, t_wall_wood);
 bay.ter_set(23, 0, t_door_c);
 bay.save();
 site = target_om_ter_random("ranch_camp_75", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.ter_set(1, 0, t_wall_wood);
 bay.ter_set(0, 0, t_door_c);
 bay.ter_set(13, 0, t_wall_wood);
 bay.ter_set(14, 0, t_door_c);
 bay.ter_set(16, 0, t_wall_wood);
 bay.ter_set(15, 0, t_door_c);
 bay.save();
 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_2(mission *miss)
{
 tripoint site = target_om_ter_random("ranch_camp_66", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.furn_set(20, 1, f_makeshift_bed);
 bay.furn_set(19, 1, f_makeshift_bed);
 bay.furn_set(20, 5, f_makeshift_bed);
 bay.furn_set(19, 5, f_makeshift_bed);
 bay.furn_set(20, 7, f_makeshift_bed);
 bay.furn_set(19, 7, f_makeshift_bed);
 bay.furn_set(20, 11, f_makeshift_bed);
 bay.furn_set(19, 11, f_makeshift_bed);
 bay.furn_set(20, 13, f_makeshift_bed);
 bay.furn_set(19, 13, f_makeshift_bed);
 bay.furn_set(20, 17, f_makeshift_bed);
 bay.furn_set(19, 17, f_makeshift_bed);
 bay.furn_set(20, 19, f_makeshift_bed);
 bay.furn_set(19, 19, f_makeshift_bed);
 bay.furn_set(20, 23, f_makeshift_bed);
 bay.furn_set(19, 23, f_makeshift_bed);
 bay.furn_set(9, 1, f_makeshift_bed);
 bay.furn_set(10, 1, f_makeshift_bed);
 bay.furn_set(9, 5, f_makeshift_bed);
 bay.furn_set(10, 5, f_makeshift_bed);
 bay.furn_set(9, 19, f_makeshift_bed);
 bay.furn_set(10, 19, f_makeshift_bed);
 bay.furn_set(9, 23, f_makeshift_bed);
 bay.furn_set(10, 23, f_makeshift_bed);
 bay.furn_set(4, 1, f_makeshift_bed);
 bay.furn_set(5, 1, f_makeshift_bed);
 bay.furn_set(4, 5, f_makeshift_bed);
 bay.furn_set(5, 5, f_makeshift_bed);
 bay.furn_set(4, 19, f_makeshift_bed);
 bay.furn_set(5, 19, f_makeshift_bed);
 bay.furn_set(4, 23, f_makeshift_bed);
 bay.furn_set(5, 23, f_makeshift_bed);
 bay.place_npc( 19, 8, string_id<npc_template>( "ranch_construction_2" ) );
 bay.save();
 site = target_om_ter_random("ranch_camp_65", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.furn_set(18, 1, f_makeshift_bed);
 bay.furn_set(19, 1, f_makeshift_bed);
 bay.furn_set(18, 5, f_makeshift_bed);
 bay.furn_set(19, 5, f_makeshift_bed);
 bay.furn_set(18, 7, f_makeshift_bed);
 bay.furn_set(19, 7, f_makeshift_bed);
 bay.furn_set(18, 11, f_makeshift_bed);
 bay.furn_set(19, 11, f_makeshift_bed);
 bay.furn_set(18, 13, f_makeshift_bed);
 bay.furn_set(19, 13, f_makeshift_bed);
 bay.furn_set(18, 17, f_makeshift_bed);
 bay.furn_set(19, 17, f_makeshift_bed);
 bay.furn_set(18, 19, f_makeshift_bed);
 bay.furn_set(19, 19, f_makeshift_bed);
 bay.furn_set(18, 23, f_makeshift_bed);
 bay.furn_set(19, 23, f_makeshift_bed);
 bay.save();
 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_3(mission *miss)
{
 tripoint site = target_om_ter_random("ranch_camp_46", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_dirt, 7, 4, 22, 23);
 bay.draw_square_ter(t_dirtmound, 8, 5, 9, 22);
 bay.draw_square_ter(t_dirtmound, 11, 5, 12, 22);
 bay.save();
 //overmap_buffer.ter(site.x, site.y, 0) = "farm_field";
 site = target_om_ter_random("ranch_camp_55", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_dirt, 7, 0, 22, 18);
 bay.draw_square_ter(t_dirtmound, 8, 2, 9, 17);
 bay.draw_square_ter(t_dirtmound, 11, 2, 12, 17);
 bay.save();
 //overmap_buffer.ter(site.x, site.y, 0) = "farm_field";
 site = target_om_ter_random("ranch_camp_66", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_npc( 4, 11, string_id<npc_template>( "ranch_woodcutter_1" ) );
 bay.save();
 site = target_om_ter_random("ranch_camp_65", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_npc( 19, 20, string_id<npc_template>( "ranch_farmer_1" ) );
 bay.furn_set(17, 11, f_bookcase);
 bay.save();
 site = target_om_ter_random("ranch_camp_56", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.add_vehicle(vproto_id("hippie_van"), 13, 20, 270);
 bay.save();
 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_4(mission *miss)
{
 tripoint site = target_om_ter_random("ranch_camp_46", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_dirtmound, 14, 5, 15, 22);
 bay.draw_square_ter(t_dirtmound, 17, 5, 18, 22);
 bay.draw_square_ter(t_dirtmound, 20, 5, 21, 22);
 bay.save();
 site = target_om_ter_random("ranch_camp_55", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_dirtmound, 14, 2, 15, 17);
 bay.draw_square_ter(t_dirtmound, 17, 2, 18, 17);
 bay.draw_square_ter(t_dirtmound, 20, 2, 21, 17);
 bay.save();
 site = target_om_ter_random("ranch_camp_58", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_dirt, 0, 3, 9, 14);
 bay.ter_set(0, 5, t_wall_log_half);
 bay.ter_set(1, 5, t_wall_log_half);
 bay.ter_set(2, 5, t_wall_log_half);
 bay.ter_set(0, 6, t_wall_log_half);
 bay.ter_set(0, 7, t_wall_log_half);
 bay.save();
 site = target_om_ter_random("ranch_camp_57", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_npc( 12, 7, string_id<npc_template>( "ranch_crop_overseer" ) );
 bay.translate(t_underbrush, t_dirt);
 bay.save();
 site = target_om_ter_random("ranch_camp_56", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.add_vehicle(vproto_id("flatbed_truck"), 20, 8, 135);
 bay.save();

 site = target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_palisade, 21, 19, 23, 19);
 bay.save();
}

void mission_start::ranch_construct_5(mission *miss)
{
 tripoint site = target_om_ter_random("ranch_camp_58", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_wall_log, 0, 5, 9, 13);
 bay.draw_square_ter(t_dirtfloor, 1, 6, 8, 12);
 bay.draw_square_ter(t_dirtfloor, 3, 5, 6, 13);
 bay.draw_square_ter(t_dirtfloor, 9, 8, 9, 10);
 bay.save();
 site = target_om_ter_random("ranch_camp_56", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.add_vehicle(vproto_id("car_chassis"), 17, 11, 90);
 bay.save();

 site = target_om_ter_random("ranch_camp_58", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.ter_set(6, 18, t_pit);
 bay.save();

 site = target_om_ter_random("ranch_camp_66", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.ter_set(23, 22, t_palisade);
 bay.place_npc( 9, 22, string_id<npc_template>( "ranch_farmer_2" ) );
 bay.save();

 site = target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_palisade, 0, 22, 5, 22);
 bay.save();
}

void mission_start::ranch_construct_6(mission *miss)
{
 tripoint site = target_om_ter_random("ranch_camp_58", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.spawn_item( 3, 9, "frame");
 bay.spawn_item( 3, 11, "frame");
 bay.spawn_item( 3, 10, "frame");
 bay.furn_set(8, 12, f_rack);
 bay.furn_set(8, 11, f_rack);
 bay.ter_set(6, 18, t_covered_well);
 bay.save();

 site = target_om_ter_random("ranch_camp_66", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.furn_set(6, 12, f_fireplace);
 bay.furn_set(8, 12, f_fireplace);
 bay.spawn_item( 11, 11, "log");
 bay.spawn_item( 11, 12, "log");
 bay.spawn_item( 11, 13, "log");
 bay.spawn_item( 3, 11, "log");
 bay.spawn_item( 3, 12, "log");
 bay.spawn_item( 3, 13, "log");
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_7(mission *miss)
{
 tripoint site = target_om_ter_random("ranch_camp_58", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.ter_set(3, 8, t_conveyor);
 bay.ter_set(3, 7, t_conveyor);
 bay.ter_set(3, 6, t_conveyor);
 bay.ter_set(3, 5, t_conveyor);
 bay.furn_set(8, 6, f_rack);
 bay.furn_set(8, 7, f_rack);
 bay.draw_square_ter(t_sidewalk, 5, 17, 7, 19);
 bay.ter_set(6, 18, t_water_pump);
 bay.save();

 site = target_om_ter_random("ranch_camp_56", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_palisade, 16, 17, 16, 23);
 bay.draw_square_ter(t_palisade, 16, 14, 19, 14);
 bay.draw_square_ter(t_palisade, 19, 11, 19, 13);
 //bay.ter_set(16, 5, t_palisade);
 bay.save();

 site = target_om_ter_random("ranch_camp_65", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_dirt, 0, 4, 12, 18);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_8(mission *miss)
{
 //Finish Sawmill
 tripoint site = target_om_ter_random("ranch_camp_58", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.spawn_item( 3, 2, "log");
 bay.spawn_item( 3, 3, "log");
 bay.spawn_item( 3, 4, "log");
 bay.spawn_item( 1, 3, "log", rng(1,5));
 bay.spawn_item( 0, 3, "log", rng(1,5));
 bay.draw_square_ter(t_machinery_old, 3, 9, 3, 10);
 bay.ter_set(3, 11, t_machinery_heavy);
 bay.spawn_item( 3, 12, "2x4", rng(1,10));
 bay.place_npc( 4, 10, string_id<npc_template>( "ranch__woodcutter_2" ) );
 bay.save();

 //Finish west wall
 site = target_om_ter_random("ranch_camp_56", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_palisade, 20, 11, 23, 11);
 bay.draw_square_ter(t_palisade, 23, 7, 23, 10);
 bay.save();

 //Finish small field to west of barn
 site = target_om_ter_random("ranch_camp_65", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_dirtmound, 1, 5, 2, 17);
 bay.draw_square_ter(t_dirtmound, 4, 5, 5, 17);
 bay.draw_square_ter(t_dirtmound, 7, 5, 8, 17);
 bay.draw_square_ter(t_dirtmound, 10, 5, 11, 17);
 bay.save();

 //Start Outhouse
 site = target_om_ter_random("ranch_camp_68", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_wall_half, 16, 1, 19, 5);
 bay.draw_square_ter(t_dirtfloor, 16, 2, 17, 2);
 bay.draw_square_ter(t_dirtfloor, 16, 4, 17, 4);
 bay.ter_set(18, 2, t_pit);
 bay.ter_set(18, 4, t_pit);
 bay.save();

 //Start Tool shed
 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_wall_half, 13, 17, 18, 21);
 bay.draw_square_ter(t_dirtfloor, 13, 19, 13, 19);
 bay.draw_square_ter(t_dirtfloor, 14, 18, 17, 20);
 bay.draw_square_ter(t_dirt, 10, 23, 12, 23);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_9(mission *miss)
{
 //Finish Outhouse
 tripoint site = target_om_ter_random("ranch_camp_68", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half,t_wall_wood);
 bay.ter_set(16, 2, t_door_c);
 bay.ter_set(16, 4, t_door_c);
 bay.place_npc( 17, 4, string_id<npc_template>( "ranch_ill_1" ) );
 bay.save();

 //Finish Tool shed
 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half,t_wall_wood);
 bay.ter_set(13, 19, t_door_c);
 bay.ter_set(18, 19, t_window_boarded_noglass);
 bay.draw_line_furn(f_counter, 14, 18, 17, 18);
 bay.draw_line_furn(f_counter, 14, 20, 17, 20);
 bay.spawn_item( 14, 18, "hammer_sledge");

 //Start Clinic
 bay.draw_square_ter(t_wall_half, 2, 3, 9, 10);
 bay.draw_square_ter(t_wall_half, 10, 5, 12, 9);
 bay.draw_square_ter(t_wall_half, 13, 3, 20, 10);
 bay.draw_square_ter(t_dirt, 3, 4, 8, 9);
 bay.draw_square_ter(t_dirt, 10, 6, 12, 8);
 bay.draw_square_ter(t_dirt, 14, 4, 19, 9);
 bay.draw_square_ter(t_dirt, 9, 7, 13, 7);
 bay.draw_square_ter(t_dirt, 5, 10, 6, 10);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_10(mission *miss)
{
 //Continue Clinic
 tripoint site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half,t_wall_wood);
 bay.ter_set(2, 5, t_window_boarded_noglass);
 bay.ter_set(2, 8, t_window_boarded_noglass);
 bay.ter_set(20, 5, t_window_boarded_noglass);
 bay.ter_set(20, 8, t_window_boarded_noglass);
 bay.spawn_item( 15, 18, "ax");
 bay.save();

 //Start Chop-Shop
 site = target_om_ter_random("ranch_camp_60", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_wall_half, 0, 9, 16, 18);
 bay.draw_square_ter(t_dirt, 1, 10, 15, 17);
 bay.draw_square_ter(t_dirt, 0, 11, 16, 16);
 bay.draw_square_ter(t_wall_half, 5, 19, 13, 21);
 bay.draw_square_ter(t_dirt, 6, 19, 8, 20);
 bay.draw_square_ter(t_dirt, 10, 19, 12, 20);
 bay.ter_set(7, 18, t_door_frame);
 bay.ter_set(11, 21, t_door_frame);
 bay.ter_set(11, 18, t_door_frame);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_11(mission *miss)
{
 //Continue Clinic
 tripoint site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_floor, 3, 4, 8, 9);
 bay.draw_square_ter(t_floor, 10, 6, 12, 8);
 bay.draw_square_ter(t_floor, 14, 4, 19, 9);
 bay.draw_square_ter(t_floor, 9, 7, 13, 7);
 bay.draw_square_ter(t_floor, 5, 10, 6, 10);
 bay.ter_set(5, 10, t_door_c);
 bay.ter_set(6, 10, t_door_c);
 bay.draw_square_furn(f_cupboard, 15, 4, 18, 4);
 bay.draw_square_furn(f_table, 16, 7, 17, 7);
 bay.place_npc( 5, 6, string_id<npc_template>( "ranch_nurse_1" ) );
 bay.spawn_item( 16, 18, "hoe");
 bay.save();

 //Continue Chop-Shop
 site = target_om_ter_random("ranch_camp_60", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half,t_wall_wood);
 bay.translate(t_door_frame,t_door_c);
 bay.save();

 //Start adding scrap vehicles
 site = target_om_ter_random("ranch_camp_61", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.add_vehicle(vproto_id("car"), 1, 20, 270);
 bay.add_vehicle(vproto_id("flatbed_truck"), 10, 23, 270);

 bay.add_vehicle(vproto_id("car_sports"), 3, 9, 90);
 bay.add_vehicle(vproto_id("cube_van_cheap"), 10, 10, 90);
 bay.save();

 //Start expanding vehicle wall
 site = target_om_ter_random("ranch_camp_69", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.add_vehicle(vproto_id("car_chassis"), 3, 14, 0);
 bay.add_vehicle(vproto_id("pickup"), 8, 15, 0);
 bay.add_vehicle(vproto_id("schoolbus"), 22, 13, 135);
 bay.save();


 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_12(mission *miss)
{
 //Finish Chop-Shop
 tripoint site = target_om_ter_random("ranch_camp_60", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_dirtfloor, 1, 10, 15, 17);
 bay.draw_square_ter(t_dirtfloor, 0, 11, 16, 16);
 bay.draw_square_ter(t_dirtfloor, 6, 19, 8, 20);
 bay.draw_square_ter(t_dirtfloor, 10, 19, 12, 20);
 bay.add_vehicle(vproto_id("armored_car"), 7, 15, 180);
 bay.draw_square_furn(f_rack, 3,10,5,10);
 bay.draw_square_furn(f_counter, 9,10,12,10);
 bay.draw_square_furn(f_rack, 10,19,10,20);
 bay.draw_square_furn(f_makeshift_bed, 8,19,8,20);
 bay.draw_square_furn(f_rack, 6,19,6,20);
 bay.place_npc( 13, 12, string_id<npc_template>( "ranch_scrapper_1" ) );
 bay.save();

 //Start Junk Shop
 site = target_om_ter_random("ranch_camp_49", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_wall_half, 0, 9, 6, 14);
 bay.draw_square_ter(t_wall_half, 6, 10, 16, 18);
 bay.draw_square_ter(t_wall_half, 8, 19, 14, 23);
 bay.draw_square_ter(t_dirt, 1, 10, 5, 13);
 bay.draw_square_ter(t_dirt, 7, 11, 15, 17);
 bay.draw_square_ter(t_dirt, 9, 18, 13, 22);
 bay.ter_set( 6, 12, t_dirt);
 bay.ter_set( 6, 16, t_dirt);
 bay.ter_set( 14, 21, t_dirt);
 bay.save();

 //Continue expanding vehicle wall
 site = target_om_ter_random("ranch_camp_69", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.add_vehicle(vproto_id("cube_van"), 13, 15, 180);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_13(mission *miss)
{
 //Continue Junk Shop
 tripoint site = target_om_ter("ranch_camp_49", 1, miss, false);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half, t_wall_wood);
 bay.ter_set(6, 12, t_dirtfloor);
 bay.ter_set(6, 16, t_door_frame);
 bay.ter_set(14, 21, t_door_frame);
 bay.draw_square_ter(t_window_frame, 0, 11, 0, 12);
 bay.draw_square_ter(t_window_frame, 10, 10, 12, 10);
 bay.draw_square_ter(t_window_frame, 16, 13, 16, 15);
 bay.ter_set(8, 21, t_window_frame);
 bay.save();

 //Continue expanding vehicle wall
 site = target_om_ter("ranch_camp_70", 1, miss, false);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.add_vehicle(vproto_id("car_mini"), 8, 3, 45);
 bay.save();

 site = target_om_ter("ranch_camp_66", 1, miss, false);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_npc( 5, 3, string_id<npc_template>( "ranch_barber" ) );
 bay.save();

 target_om_ter("ranch_camp_67", 1, miss, false);
}

void mission_start::ranch_construct_14(mission *miss)
{
 //Finish Junk Shop
 tripoint site = target_om_ter_random("ranch_camp_49", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_door_frame, t_door_c);
 bay.translate(t_window_frame, t_window_boarded_noglass);
 bay.draw_square_ter(t_dirtfloor, 1, 10, 5, 13);
 bay.draw_square_ter(t_dirtfloor, 7, 11, 15, 17);
 bay.draw_square_ter(t_dirtfloor, 9, 18, 13, 22);
 bay.draw_square_furn(f_counter, 11, 20, 11, 22);
 bay.draw_square_furn(f_rack, 15, 11, 15, 17);
 bay.draw_square_furn(f_rack, 1, 10, 5, 10);
 bay.draw_square_furn(f_rack, 1, 13, 5, 13);
 bay.draw_square_furn(f_counter, 9, 13, 10, 16);
 bay.draw_square_furn(f_counter, 12, 13, 13, 16);
 bay.place_npc( 10, 21, string_id<npc_template>( "ranch_scavenger_1" ) );
 bay.save();

 //Start Bar
 site = target_om_ter_random("ranch_camp_51", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_wall_half, 0, 18, 14, 23);
 bay.draw_square_ter(t_dirt, 1, 19, 13, 23);
 bay.draw_square_ter(t_wall_half, 14, 18, 19, 22);
 bay.draw_square_ter(t_dirt, 15, 19, 18, 21);
 bay.draw_square_ter(t_wall_half, 7, 15, 10, 18);
 bay.draw_square_ter(t_dirt, 8, 16, 9, 17);
 bay.draw_square_ter(t_wall_half, 10, 15, 14, 18);
 bay.draw_square_ter(t_dirt, 11, 16, 13, 17);
 bay.ter_set(8,18,t_door_frame);
 bay.ter_set(12,18,t_door_frame);
 bay.ter_set(14,20,t_door_frame);
 bay.save();
 site = target_om_ter_random("ranch_camp_60", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_wall_half, 0, 0, 14, 1);
 bay.draw_square_ter(t_wall_half, 3, 1, 11, 4);
 bay.draw_square_ter(t_dirt, 1, 0, 13, 0);
 bay.draw_square_ter(t_dirt, 4, 1, 10, 3);
 bay.draw_square_ter(t_door_frame, 3, 2, 3, 3);
 bay.save();

 //Continue expanding vehicle wall
 site = target_om_ter_random("ranch_camp_61", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.add_vehicle(vproto_id("ambulance"), 14, 4, 90);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_15(mission *miss)
{
 //Continue Bar
 tripoint site = target_om_ter_random("ranch_camp_51", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half, t_wall_wood);
 bay.draw_square_ter(t_window_frame, 0, 21, 0, 22);
 bay.draw_square_ter(t_window_frame, 3, 18, 4, 18);
 bay.save();
 site = target_om_ter_random("ranch_camp_60", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half, t_wall_wood);
 bay.ter_set(7,4,t_window_frame);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_construct_16(mission *miss)
{
 //Finish Bar
 tripoint site = target_om_ter_random("ranch_camp_51", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_window_frame, t_window_boarded_noglass);
 bay.translate(t_door_frame, t_door_c);
 bay.draw_square_ter(t_dirtfloor, 1, 19, 13, 23);
 bay.draw_square_ter(t_dirtfloor, 15, 19, 18, 21);
 bay.draw_square_ter(t_dirtfloor, 8, 16, 9, 17);
 bay.draw_square_ter(t_dirtfloor, 11, 16, 13, 17);
 bay.draw_square_furn(f_counter, 11, 20, 11, 23);
 bay.draw_square_furn(f_chair, 1, 20, 2, 23);
 bay.draw_square_furn(f_table, 1, 21, 2, 22);
 bay.draw_square_furn(f_chair, 5, 23, 7, 23);
 bay.draw_square_furn(f_table, 6, 23, 6, 23);
 bay.draw_square_furn(f_chair, 5, 20, 7, 20);
 bay.draw_square_furn(f_table, 6, 20, 6, 20);
 bay.draw_square_furn(f_rack, 13, 21, 13, 22);
 bay.draw_square_furn(f_rack, 11, 16, 11, 17);
 bay.draw_square_furn(f_wood_keg, 16, 19, 17, 19);
 bay.draw_square_furn(f_fvat_empty, 16, 21, 17, 21);
 //Do a check to prevent duplicate NPCs in the last mission of each version
    const std::vector<std::shared_ptr<npc>> all_npcs = overmap_buffer.get_npcs_near( site.x*2,site.y*2,site.z,3 );
 bool already_has = false;
 unsigned int a = -1;
 for( auto &elem : all_npcs ) {
    if (elem->name.find(", Bartender") != a){
        already_has = true;
    }
 }
    if( !already_has ) {
    bay.place_npc( 12, 22, string_id<npc_template>( "ranch_bartender" ) );
    bay.place_npc( 7, 20, string_id<npc_template>( "scavenger_merc" ) );
 }
 bay.save();

 site = target_om_ter_random("ranch_camp_60", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_window_frame, t_window_boarded_noglass);
 bay.translate(t_door_frame, t_door_c);
 bay.draw_square_ter(t_dirtfloor, 1, 0, 13, 0);
 bay.draw_square_ter(t_dirtfloor, 4, 1, 10, 3);
 bay.furn_set(11,0,f_counter);
 bay.save();

 //Start Greenhouse
 site = target_om_ter_random("ranch_camp_52", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_underbrush, t_dirt);
 bay.draw_square_ter(t_dirt, 1, 9, 13, 23);
 bay.draw_square_ter(t_wall_half, 2, 10, 12, 22);
 bay.draw_square_ter(t_door_frame, 7, 10, 7, 22);
 bay.draw_square_ter(t_dirt, 3, 11, 11, 21);
 bay.save();

 target_om_ter_random("ranch_camp_67", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_nurse_1(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_furn(f_rack, 16, 9, 17, 9);
 bay.spawn_item( 16, 9, "bandages", rng(1,3));
 bay.spawn_item( 17, 9, "aspirin", rng(1,2));
 bay.save();
}

void mission_start::ranch_nurse_2(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_furn(f_counter, 3, 7, 5, 7);
 bay.draw_square_furn(f_rack, 8, 4, 8, 5);
 bay.spawn_item( 8, 4, "manual_first_aid");
 bay.save();
}

void mission_start::ranch_nurse_3(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_50", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_dirt, 2, 16, 9, 23);
 bay.draw_square_ter(t_dirt, 13, 16, 20, 23);
 bay.draw_square_ter(t_dirt, 10, 17, 12, 23);
 bay.save();

 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_dirt, 2, 0, 20, 2);
 bay.draw_square_ter(t_dirt, 10, 3, 12, 4);
 bay.save();
}

void mission_start::ranch_nurse_4(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_50", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_wall_half, 2, 16, 9, 23);
 bay.draw_square_ter(t_dirt, 3, 17, 8, 22);
 bay.draw_square_ter(t_wall_half, 13, 16, 20, 23);
 bay.draw_square_ter(t_dirt, 14, 17, 19, 22);
 bay.draw_square_ter(t_wall_half, 10, 17, 12, 23);
 bay.draw_square_ter(t_dirt, 10, 18, 12, 23);
 bay.ter_set( 9, 19, t_door_frame);
 bay.ter_set( 13, 19, t_door_frame);
 bay.save();

 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_wall_half, 4, 0, 18, 2);
 bay.draw_square_ter(t_wall_half, 10, 3, 12, 4);
 bay.draw_square_ter(t_dirt, 5, 0, 8, 2);
 bay.draw_square_ter(t_dirt, 10, 0, 12, 4);
 bay.draw_square_ter(t_dirt, 14, 0, 17, 2);
 bay.ter_set( 9, 1, t_door_frame);
 bay.ter_set( 13, 1, t_door_frame);
 bay.save();
}

void mission_start::ranch_nurse_5(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_50", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half, t_wall_wood);
 bay.ter_set( 2, 21, t_window_frame);
 bay.ter_set( 2, 18, t_window_frame);
 bay.ter_set( 20, 18, t_window_frame);
 bay.ter_set( 20, 21, t_window_frame);
 bay.ter_set( 11, 17, t_window_frame);
 bay.save();

 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_wall_half, t_wall_wood);
 bay.draw_square_ter(t_dirt, 10, 0, 12, 4);
 bay.save();
}

void mission_start::ranch_nurse_6(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_50", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_window_frame, t_window_boarded_noglass);
 bay.translate(t_door_frame, t_door_c);
 bay.draw_square_ter(t_dirtfloor, 3, 17, 8, 22);
 bay.draw_square_ter(t_dirtfloor, 14, 17, 19, 22);
 bay.draw_square_ter(t_dirtfloor, 10, 18, 12, 23);
 bay.save();

 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_door_frame, t_door_c);
 bay.draw_square_ter(t_dirtfloor, 5, 0, 8, 2);
 bay.draw_square_ter(t_dirtfloor, 10, 0, 12, 4);
 bay.draw_square_ter(t_dirtfloor, 14, 0, 17, 2);
 bay.save();
}

void mission_start::ranch_nurse_7(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_50", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_dirtfloor, t_floor);
 bay.save();

 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_dirtfloor, t_floor);
 bay.draw_square_ter(t_floor, 10, 5, 12, 5);
 bay.draw_square_furn(f_rack, 17, 0, 17, 2);
 bay.save();
}

void mission_start::ranch_nurse_8(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_50", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_furn(f_makeshift_bed, 4, 21, 4, 22);
 bay.draw_square_furn(f_makeshift_bed, 7, 21, 7, 22);
 bay.draw_square_furn(f_makeshift_bed, 15, 21, 15, 22);
 bay.draw_square_furn(f_makeshift_bed, 18, 21, 18, 22);
 bay.draw_square_furn(f_makeshift_bed, 4, 17, 4, 18);
 bay.draw_square_furn(f_makeshift_bed, 7, 17, 7, 18);
 bay.draw_square_furn(f_makeshift_bed, 15, 17, 15, 18);
 bay.draw_square_furn(f_makeshift_bed, 18, 17, 18, 18);
 bay.save();

 site = target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_dirtfloor, t_floor);
 bay.place_items( "cleaning", 75, 17, 0, 17, 2, true, 0 );
 bay.place_items( "surgery", 75, 15, 4, 18, 4, true, 0 );
 bay.save();
}

void mission_start::ranch_nurse_9(mission *miss)
{
 //Improvements to clinic...
 tripoint site = target_om_ter_random("ranch_camp_50", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.furn_set(3, 22, f_dresser);
 bay.furn_set(8, 22, f_dresser);
 bay.furn_set(14, 22, f_dresser);
 bay.furn_set(19, 22, f_dresser);
 bay.furn_set(3, 17, f_dresser);
 bay.furn_set(8, 17, f_dresser);
 bay.furn_set(14, 17, f_dresser);
 bay.furn_set(19, 17, f_dresser);
 bay.place_npc( 16, 19, string_id<npc_template>( "ranch_doctor" ) );
 bay.save();

 target_om_ter_random("ranch_camp_59", 1, miss, false, RANCH_SIZE);
}

void mission_start::ranch_scavenger_1(mission *miss)
{
 npc *p = g->find_npc( miss->npc_id );
 p->my_fac->combat_ability += rng(1,2);

 tripoint site = target_om_ter_random("ranch_camp_48", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter(t_chainfence, 15, 13, 15, 22);
 bay.draw_square_ter(t_chainfence, 16, 13, 23, 13);
 bay.draw_square_ter(t_chainfence, 16, 22, 23, 22);
 bay.save();

 site = target_om_ter_random("ranch_camp_49", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_items( "mechanics", 65, 9, 13, 10, 16, true, 0 );
 bay.draw_square_ter(t_chainfence, 0, 22, 7, 22);
 bay.draw_square_ter(t_dirt, 2, 22, 3, 22);
 bay.spawn_item( 7, 19, "30gal_drum" );
 bay.save();
}

void mission_start::ranch_scavenger_2(mission *miss)
{
 npc *p = g->find_npc( miss->npc_id );
 p->my_fac->combat_ability += rng(1,2);

 tripoint site = target_om_ter_random("ranch_camp_48", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.add_vehicle(vproto_id("car_chassis"), 20, 15, 0);
 bay.draw_square_ter(t_wall_half, 18, 19, 21, 22);
 bay.draw_square_ter(t_dirt, 19, 20, 20, 21);
 bay.ter_set( 19, 19, t_door_frame);
 bay.save();

 site = target_om_ter_random("ranch_camp_49", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_items( "mischw", 65, 12, 13, 13, 16, true, 0 );
 bay.draw_square_ter(t_chaingate_l, 2, 22, 3, 22);
 bay.spawn_item( 7, 20, "30gal_drum" );
 bay.save();
}

void mission_start::ranch_scavenger_3(mission *miss)
{
 npc *p = g->find_npc( miss->npc_id );
 p->my_fac->combat_ability += rng(1,2);

 tripoint site = target_om_ter_random("ranch_camp_48", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate(t_door_frame, t_door_locked);
 bay.translate(t_wall_half, t_wall_wood);
 bay.draw_square_ter(t_dirtfloor, 19, 20, 20, 21);
 bay.spawn_item( 16, 21, "wheel_wide" );
 bay.spawn_item( 17, 21, "wheel_wide" );
 bay.spawn_item( 23, 18, "v8_combustion" );
 bay.furn_set(23,17, furn_str_id( "f_arcade_machine" ) );
 bay.ter_set(23,16, ter_str_id( "t_machinery_light" ) );
 bay.furn_set( 20, 21, f_woodstove);
 bay.save();

 site = target_om_ter_random("ranch_camp_49", 1, miss, false, RANCH_SIZE);
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.place_items( "mischw", 65, 2, 10, 4, 10, true, 0 );
 bay.place_items( "mischw", 65, 2, 13, 4, 13, true, 0 );
 bay.furn_set( 1, 15, f_fridge);
 bay.spawn_item( 2, 15, "hdframe" );
 bay.furn_set(3,15,f_washer);
 bay.save();
}

void mission_start::ranch_bartender_1(mission *miss)
{
 npc *p = g->find_npc( miss->npc_id );
 p->my_fac->wealth += rng(500,2500);
 p->set_mutation( trait_id( "NPC_BRANDY" ) );

 tripoint site = target_om_ter_random("ranch_camp_51", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter( t_wall_half, 1,15,7,17);
 bay.draw_square_ter( t_dirt, 2,16,6,17);
 bay.draw_square_ter( t_wall_half, 0,8,14,15);
 bay.draw_square_ter( t_dirt, 1, 9, 13, 14);
 bay.ter_set( 0, 10, t_door_frame);
 bay.ter_set( 0, 11, t_door_frame);
 bay.draw_square_ter( t_dirtfloor, 3, 15, 5, 15);
 bay.draw_square_ter( t_dirtfloor, 3, 18, 5, 18);
 //bay.draw_square_ter( t_dirtfloor, 15, 18, 16, 18);

 bay.save();
}

void mission_start::ranch_bartender_2(mission *miss)
{
 npc *p = g->find_npc( miss->npc_id );
 p->my_fac->wealth += rng(500,2500);
 p->set_mutation( trait_id( "NPC_RUM" ) );

 tripoint site = target_om_ter_random("ranch_camp_51", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_ter( t_wall_half, 14,10,19,15);
 bay.draw_square_ter( t_dirt, 15,11,18,14);
 bay.draw_square_ter( t_wall_half, 14,15,17,18);
 bay.draw_square_ter( t_dirt, 15,15,16,18);;
 bay.translate( t_door_frame, t_door_c);
 bay.translate( t_wall_half, t_wall_wood);
 bay.draw_square_ter( t_window_frame, 0, 13, 0, 13);
 bay.draw_square_ter( t_window_frame, 4, 8, 5, 8);
 bay.draw_square_ter( t_window_frame, 9, 8, 10, 8);
 bay.draw_square_ter( t_window_frame, 19, 12, 19, 12);
 bay.furn_set( 18, 19, f_wood_keg);
 bay.furn_set( 16, 19, f_null);
 bay.save();
}

void mission_start::ranch_bartender_3(mission *miss)
{
 npc *p = g->find_npc( miss->npc_id );
 p->my_fac->wealth += rng(500,2500);
 p->set_mutation( trait_id( "NPC_WHISKEY" ) );

 tripoint site = target_om_ter_random("ranch_camp_51", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.translate( t_window_frame, t_window_boarded_noglass);
 bay.draw_square_ter( t_dirtfloor, 15,11,18,14);
 bay.draw_square_ter( t_dirtfloor, 15,15,16,18);
 bay.draw_square_ter( t_dirtfloor, 1, 9, 13, 14);
 bay.draw_square_ter( t_dirtfloor, 15,11,18,14);
 bay.save();
}

void mission_start::ranch_bartender_4(mission *miss)
{
 npc *p = g->find_npc( miss->npc_id );
 p->my_fac->wealth += rng(500,2500);

 tripoint site = target_om_ter_random("ranch_camp_51", 1, miss, false, RANCH_SIZE);
 tinymap bay;
 bay.load(site.x * 2, site.y * 2, site.z, false);
 bay.draw_square_furn(f_standing_tank, 16, 11, 17, 11);
 bay.draw_square_furn(f_fvat_empty, 17, 14, 18, 14);
 bay.draw_square_furn(f_standing_tank, 13, 23, 13, 23);
 bay.draw_square_furn(f_chair, 4, 10, 6, 10);
 bay.draw_square_furn(f_table, 5, 10, 5, 10);
 bay.draw_square_furn(f_chair, 5, 13, 7, 13);
 bay.draw_square_furn(f_table, 6, 13, 6, 13);

 bay.draw_square_furn(f_chair, 10, 10, 11, 10);
 bay.draw_square_furn(f_table, 10, 11, 11, 12);
 bay.draw_square_furn(f_chair, 10, 13, 11, 13);
 bay.draw_square_furn(f_chair, 9, 11, 9, 12);
 bay.draw_square_furn(f_chair, 12, 11, 12, 12);
 bay.save();
}

void mission_start::place_book( mission *)
{
}

const tripoint reveal_destination( const std::string &type )
{
    const tripoint your_pos = g->u.global_omt_location();
    const tripoint center_pos = overmap_buffer.find_random( your_pos, type, rng( 40, 80 ), false );

    if( center_pos != overmap::invalid_tripoint ) {
        overmap_buffer.reveal( center_pos, 2 );
        return center_pos;
    }

    return overmap::invalid_tripoint;
}

void reveal_route( mission *miss, const tripoint destination )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "mission_start::infect_npc() couldn't find an NPC!" );
        return;
    }

    const tripoint source = g->u.global_omt_location();

    const tripoint source_road = overmap_buffer.find_closest( source, "road", 3, false );
    const tripoint dest_road = overmap_buffer.find_closest( destination, "road", 3, false );

    if( overmap_buffer.reveal_route( source_road, dest_road ) ) {
        add_msg( _( "%s also marks the road that leads to it..." ),
                 p->name.c_str() );
    }
}

void reveal_target( mission *miss, const std::string &omter_id )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "mission_start::infect_npc() couldn't find an NPC!" );
        return;
    }

    const tripoint destination = reveal_destination( omter_id );
    if( destination != overmap::invalid_tripoint ) {
        const oter_id oter = overmap_buffer.ter( destination );
        add_msg( _( "%s has marked the only %s known to them on your map." ),
                 p->name.c_str(), oter->get_name().c_str() );
        miss->set_target( destination );
        if( one_in( 3 ) ) {
            reveal_route( miss, destination );
        }
    }
}

void reveal_any_target( mission *miss, const std::vector<std::string> &omter_ids )
{
    reveal_target( miss, random_entry( omter_ids ).c_str() );
}

void mission_start::reveal_weather_station( mission *miss )
{
    reveal_target( miss, "station_radio" );
}

void mission_start::reveal_office_tower( mission *miss )
{
    reveal_target( miss, "office_tower_1" );
}

void mission_start::reveal_doctors_office( mission *miss )
{
    reveal_any_target( miss, { "office_doctor", "hospital_2" } );
}

void mission_start::reveal_cathedral( mission *miss )
{
    reveal_any_target( miss, { "cathedral_1", "museum" } );
}

void mission_start::reveal_refugee_center( mission *miss )
{
    const tripoint your_pos = g->u.global_omt_location();
    const tripoint center_pos = overmap_buffer.find_closest( your_pos, "evac_center_18", 0, false );

    if( center_pos == overmap::invalid_tripoint ) {
        add_msg( _( "You don't know where the address could be..." ) );
        return;
    }

    miss->set_target( center_pos );

    if( overmap_buffer.seen( center_pos.x, center_pos.y, center_pos.z ) ) {
        add_msg( _( "You already know that address..." ) );
        return;
    }

    add_msg( _( "It takes you forever to find the address on your map..." ) );

    overmap_buffer.reveal( center_pos, 3 );

    const tripoint source_road = overmap_buffer.find_closest( your_pos, "road", 3, false );
    const tripoint dest_road = overmap_buffer.find_closest( center_pos, "road", 3, false );

    if( overmap_buffer.reveal_route( source_road, dest_road, 1, true ) ) {
        add_msg( _( "You mark the refugee center and the road that leads to it..." ) );
    } else {
        add_msg( _( "You mark the refugee center, but you have no idea how to get there by road..." ) );
    }
}
