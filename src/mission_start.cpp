#include "mission.h" // IWYU pragma: associated

#include <cstdio>
#include <vector>

#include "computer.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "dialogue.h"
#include "field.h"
#include "game.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
// TODO: Remove this include once 2D wrappers are no longer needed
#include "mapgen_functions.h"
#include "messages.h"
#include "name.h"
#include "npc.h"
#include "npc_class.h"
#include "omdata.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"

class DynamicDataLoader;

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

/**
 * Given a (valid!) city reference, select a random house within the city borders.
 * @return global overmap terrain coordinates of the house.
 */
static tripoint random_house_in_city( const city_reference &cref )
{
    const auto city_center_omt = sm_to_omt_copy( cref.abs_sm_pos );
    const auto size = cref.city->size;
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

static tripoint target_closest_lab_entrance( const tripoint &origin, int reveal_rad, mission *miss )
{
    tripoint testpoint = tripoint( origin );
    // Get the surface locations for labs and for spaces above hidden lab stairs.
    testpoint.z = 0;
    tripoint surface = overmap_buffer.find_closest( testpoint, "lab_stairs", 0, false, true );

    testpoint.z = -1;
    tripoint underground = overmap_buffer.find_closest( testpoint, "hidden_lab_stairs", 0, false,
                           true );
    underground.z = 0;

    tripoint closest;
    if( square_dist( surface.x, surface.y, origin.x, origin.y ) <= square_dist( underground.x,
            underground.y, origin.x, origin.y ) ) {
        closest = surface;
    } else {
        closest = underground;
    }

    if( closest != overmap::invalid_tripoint && reveal_rad >= 0 ) {
        overmap_buffer.reveal( closest, reveal_rad );
    }
    miss->set_target( closest );
    return closest;
}

static bool reveal_road( const tripoint &source, const tripoint &dest, overmapbuffer &omb )
{
    const tripoint source_road = overmap_buffer.find_closest( source, "road", 3, false );
    const tripoint dest_road = overmap_buffer.find_closest( dest, "road", 3, false );
    return omb.reveal_route( source_road, dest_road );
}

struct mission_target_params {
    std::string overmap_terrain_subtype;
    mission *mission_pointer;

    cata::optional<tripoint> search_origin;
    cata::optional<std::string> replaceable_overmap_terrain_subtype;
    cata::optional<overmap_special_id> overmap_special;
    cata::optional<int> reveal_radius;

    bool must_see = false;
    bool random = false;
    bool create_if_necessary = true;
    int search_range = OMAPX;
};

static cata::optional<tripoint> assign_mission_target( const mission_target_params &params )
{
    // If a search origin is provided, then use that. Otherwise, use the player's current
    // location.
    const tripoint origin_pos = params.search_origin ? *params.search_origin :
                                g->u.global_omt_location();

    tripoint target_pos = overmap::invalid_tripoint;

    // Either find a random or closest match, based on the criteria.
    if( params.random ) {
        target_pos = overmap_buffer.find_random( origin_pos, params.overmap_terrain_subtype,
                     params.search_range, params.must_see, false, true, params.overmap_special );
    } else {
        target_pos = overmap_buffer.find_closest( origin_pos, params.overmap_terrain_subtype,
                     params.search_range, params.must_see, false, true, params.overmap_special );
    }

    // If we didn't find a match, and we're allowed to create new terrain, and the player didn't
    // have to see the location beforehand, then we can attempt to create the new terrain.
    if( target_pos == overmap::invalid_tripoint && params.create_if_necessary && !params.must_see ) {
        // If this terrain is part of an overmap special...
        if( params.overmap_special ) {
            // ...then attempt to place the whole special.
            const bool placed = overmap_buffer.place_special( *params.overmap_special, origin_pos,
                                params.search_range );
            // If we succeeded in placing the special, then try and find the particular location
            // we're interested in.
            if( placed ) {
                target_pos = overmap_buffer.find_closest( origin_pos, params.overmap_terrain_subtype,
                             params.search_range, false, false, true, params.overmap_special );
            }
        } else if( params.replaceable_overmap_terrain_subtype ) {
            // This terrain wasn't part of an overmap special, but we do have a replacement
            // terrain specified. Find a random location of that replacement type.
            target_pos = overmap_buffer.find_random( origin_pos,
                         *params.replaceable_overmap_terrain_subtype,
                         params.search_range, false, false, true );

            // We didn't find it, so allow this search to create new overmaps and try again.
            if( target_pos == overmap::invalid_tripoint ) {
                target_pos = overmap_buffer.find_random( origin_pos,
                             *params.replaceable_overmap_terrain_subtype,
                             params.search_range, false, false, false );
            }

            // We found a match, so set this position (which was our replacement terrain)
            // to our desired mission terrain.
            if( target_pos != overmap::invalid_tripoint ) {
                overmap_buffer.ter( target_pos ) = oter_id( params.overmap_terrain_subtype );
            }
        }
    }

    // If we got here and this is still invalid, it means that we couldn't find it and (if
    // allowed by the parameters) we couldn't create it either.
    if( target_pos == overmap::invalid_tripoint ) {
        debugmsg( "Unable to find and assign mission target %s.", params.overmap_terrain_subtype );
        return cata::nullopt;
    }

    // If we specified a reveal radius, then go ahead and reveal around our found position.
    if( params.reveal_radius ) {
        overmap_buffer.reveal( target_pos, *params.reveal_radius );
    }

    // Set the mission target to our found position.
    params.mission_pointer->set_target( target_pos );

    return target_pos;
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
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
        return;
    }
    p->add_effect( effect_infection, 1_turns, num_bp, true, 1 );
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
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
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
    if( dev == nullptr ) {
        debugmsg( "Couldn't find NPC! %d", miss->npc_id );
        return;
    }
    g->u.i_add( item( "dog_whistle", 0 ) );
    add_msg( _( "%s gave you a dog whistle." ), dev->name );

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

void mission_start::place_jabberwock( mission *miss )
{
    tripoint site = mission_util::target_om_ter( "forest_thick", 6, miss, false );
    tinymap grove;
    grove.load( site.x * 2, site.y * 2, site.z, false );
    grove.add_spawn( mon_jabberwock, 1, SEEX, SEEY, false, -1, miss->uid, "NONE" );
    grove.save();
}

void mission_start::kill_20_nightmares( mission *miss )
{
    mission_util::target_om_ter( "necropolis_c_44", 3, miss, false, -2 );
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

    if( overmap::inbounds( tripoint( SEEX, SEEY, 0 ), 1 ) ) {
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

/*
 * Find a location to place a computer.  In order, prefer:
 * 1) Broken consoles.
 * 2) Corners or coords adjacent to a bed/dresser? (this logic may be flawed, dates from Whales in 2011)
 * 3) A random spot near the center of the tile.
 */
static tripoint find_potential_computer_point( const tinymap &compmap, int z )
{
    std::vector<tripoint> broken;
    std::vector<tripoint> potential;
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            if( compmap.ter( x, y ) == t_console_broken ) {
                broken.push_back( tripoint( x, y, z ) );
            } else if( compmap.ter( x, y ) == t_floor && compmap.furn( x, y ) == f_null ) {
                bool okay = false;
                int wall = 0;
                for( int x2 = x - 1; x2 <= x + 1 && !okay; x2++ ) {
                    for( int y2 = y - 1; y2 <= y + 1 && !okay; y2++ ) {
                        if( compmap.furn( x2, y2 ) == f_bed || compmap.furn( x2, y2 ) == f_dresser ) {
                            okay = true;
                            potential.push_back( tripoint( x, y, z ) );
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
                        potential.push_back( tripoint( x, y, z ) );
                    }
                }
            }
        }
    }
    const tripoint fallback( rng( 10, SEEX * 2 - 11 ), rng( 10, SEEY * 2 - 11 ), z );
    return random_entry( !broken.empty() ? broken : potential, fallback );
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
        comppoint = find_potential_computer_point( compmap, place.z );
    }

    compmap.ter_set( comppoint, t_console );
    computer *tmpcomp = compmap.add_computer( comppoint, string_format( _( "%s's Terminal" ),
                        dev->name.c_str() ), 0 );
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

void mission_start::recruit_tracker( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->set_attitude( NPCATT_FOLLOW );// NPC joins you

    tripoint site = mission_util::target_om_ter( "cabin", 2, miss, false );
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
    temp->add_new_mission( mission::reserve_new( mission_type_id( "MISSION_JOIN_TRACKER" ),
                           temp->getID() ) );
}

const int RANCH_SIZE = 5;

void mission_start::ranch_construct_1( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_66", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.ter_set( 1, 0, t_wall_wood );
    bay.ter_set( 0, 0, t_door_c );
    bay.ter_set( 13, 0, t_wall_wood );
    bay.ter_set( 14, 0, t_door_c );
    bay.ter_set( 16, 0, t_wall_wood );
    bay.ter_set( 15, 0, t_door_c );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_65", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.ter_set( 22, 0, t_wall_wood );
    bay.ter_set( 23, 0, t_door_c );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_74", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.ter_set( 22, 0, t_wall_wood );
    bay.ter_set( 23, 0, t_door_c );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_75", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.ter_set( 1, 0, t_wall_wood );
    bay.ter_set( 0, 0, t_door_c );
    bay.ter_set( 13, 0, t_wall_wood );
    bay.ter_set( 14, 0, t_door_c );
    bay.ter_set( 16, 0, t_wall_wood );
    bay.ter_set( 15, 0, t_door_c );
    bay.save();
    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_2( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_66", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.furn_set( 20, 1, f_makeshift_bed );
    bay.furn_set( 19, 1, f_makeshift_bed );
    bay.furn_set( 20, 5, f_makeshift_bed );
    bay.furn_set( 19, 5, f_makeshift_bed );
    bay.furn_set( 20, 7, f_makeshift_bed );
    bay.furn_set( 19, 7, f_makeshift_bed );
    bay.furn_set( 20, 11, f_makeshift_bed );
    bay.furn_set( 19, 11, f_makeshift_bed );
    bay.furn_set( 20, 13, f_makeshift_bed );
    bay.furn_set( 19, 13, f_makeshift_bed );
    bay.furn_set( 20, 17, f_makeshift_bed );
    bay.furn_set( 19, 17, f_makeshift_bed );
    bay.furn_set( 20, 19, f_makeshift_bed );
    bay.furn_set( 19, 19, f_makeshift_bed );
    bay.furn_set( 20, 23, f_makeshift_bed );
    bay.furn_set( 19, 23, f_makeshift_bed );
    bay.furn_set( 9, 1, f_makeshift_bed );
    bay.furn_set( 10, 1, f_makeshift_bed );
    bay.furn_set( 9, 5, f_makeshift_bed );
    bay.furn_set( 10, 5, f_makeshift_bed );
    bay.furn_set( 9, 19, f_makeshift_bed );
    bay.furn_set( 10, 19, f_makeshift_bed );
    bay.furn_set( 9, 23, f_makeshift_bed );
    bay.furn_set( 10, 23, f_makeshift_bed );
    bay.furn_set( 4, 1, f_makeshift_bed );
    bay.furn_set( 5, 1, f_makeshift_bed );
    bay.furn_set( 4, 5, f_makeshift_bed );
    bay.furn_set( 5, 5, f_makeshift_bed );
    bay.furn_set( 4, 19, f_makeshift_bed );
    bay.furn_set( 5, 19, f_makeshift_bed );
    bay.furn_set( 4, 23, f_makeshift_bed );
    bay.furn_set( 5, 23, f_makeshift_bed );
    bay.place_npc( 19, 8, string_id<npc_template>( "ranch_construction_2" ) );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_65", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.furn_set( 18, 1, f_makeshift_bed );
    bay.furn_set( 19, 1, f_makeshift_bed );
    bay.furn_set( 18, 5, f_makeshift_bed );
    bay.furn_set( 19, 5, f_makeshift_bed );
    bay.furn_set( 18, 7, f_makeshift_bed );
    bay.furn_set( 19, 7, f_makeshift_bed );
    bay.furn_set( 18, 11, f_makeshift_bed );
    bay.furn_set( 19, 11, f_makeshift_bed );
    bay.furn_set( 18, 13, f_makeshift_bed );
    bay.furn_set( 19, 13, f_makeshift_bed );
    bay.furn_set( 18, 17, f_makeshift_bed );
    bay.furn_set( 19, 17, f_makeshift_bed );
    bay.furn_set( 18, 19, f_makeshift_bed );
    bay.furn_set( 19, 19, f_makeshift_bed );
    bay.furn_set( 18, 23, f_makeshift_bed );
    bay.furn_set( 19, 23, f_makeshift_bed );
    bay.save();
    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_3( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_46", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_dirt, 7, 4, 22, 23 );
    bay.draw_square_ter( t_dirtmound, 8, 5, 9, 22 );
    bay.draw_square_ter( t_dirtmound, 11, 5, 12, 22 );
    bay.save();
    //overmap_buffer.ter(site.x, site.y, 0) = "farm_field";
    site = mission_util::target_om_ter_random( "ranch_camp_55", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_dirt, 7, 0, 22, 18 );
    bay.draw_square_ter( t_dirtmound, 8, 2, 9, 17 );
    bay.draw_square_ter( t_dirtmound, 11, 2, 12, 17 );
    bay.save();
    //overmap_buffer.ter(site.x, site.y, 0) = "farm_field";
    site = mission_util::target_om_ter_random( "ranch_camp_66", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.place_npc( 4, 11, string_id<npc_template>( "ranch_woodcutter_1" ) );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_65", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.place_npc( 19, 20, string_id<npc_template>( "ranch_farmer_1" ) );
    bay.furn_set( 17, 11, f_bookcase );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_56", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.add_vehicle( vproto_id( "hippie_van" ), 13, 20, 270 );
    bay.save();
    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_4( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_46", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_dirtmound, 14, 5, 15, 22 );
    bay.draw_square_ter( t_dirtmound, 17, 5, 18, 22 );
    bay.draw_square_ter( t_dirtmound, 20, 5, 21, 22 );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_55", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_dirtmound, 14, 2, 15, 17 );
    bay.draw_square_ter( t_dirtmound, 17, 2, 18, 17 );
    bay.draw_square_ter( t_dirtmound, 20, 2, 21, 17 );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_58", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_dirt, 0, 3, 9, 14 );
    bay.ter_set( 0, 5, t_wall_log_half );
    bay.ter_set( 1, 5, t_wall_log_half );
    bay.ter_set( 2, 5, t_wall_log_half );
    bay.ter_set( 0, 6, t_wall_log_half );
    bay.ter_set( 0, 7, t_wall_log_half );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_57", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.place_npc( 12, 7, string_id<npc_template>( "ranch_crop_overseer" ) );
    bay.translate( t_underbrush, t_dirt );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_56", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.add_vehicle( vproto_id( "flatbed_truck" ), 20, 8, 135 );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_palisade, 21, 19, 23, 19 );
    bay.save();
}

void mission_start::ranch_construct_5( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_58", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_wall_log, 0, 5, 9, 13 );
    bay.draw_square_ter( t_dirtfloor, 1, 6, 8, 12 );
    bay.draw_square_ter( t_dirtfloor, 3, 5, 6, 13 );
    bay.draw_square_ter( t_dirtfloor, 9, 8, 9, 10 );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_56", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.add_vehicle( vproto_id( "car_chassis" ), 17, 11, 90 );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_58", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.ter_set( 6, 18, t_pit );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_66", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.ter_set( 23, 22, t_palisade );
    bay.place_npc( 9, 22, string_id<npc_template>( "ranch_farmer_2" ) );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_palisade, 0, 22, 5, 22 );
    bay.save();
}

void mission_start::ranch_construct_6( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_58", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.spawn_item( 3, 9, "frame" );
    bay.spawn_item( 3, 11, "frame" );
    bay.spawn_item( 3, 10, "frame" );
    bay.furn_set( 8, 12, f_rack );
    bay.furn_set( 8, 11, f_rack );
    bay.ter_set( 6, 18, t_covered_well );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_66", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.furn_set( 6, 12, f_fireplace );
    bay.furn_set( 8, 12, f_fireplace );
    bay.spawn_item( 11, 11, "log" );
    bay.spawn_item( 11, 12, "log" );
    bay.spawn_item( 11, 13, "log" );
    bay.spawn_item( 3, 11, "log" );
    bay.spawn_item( 3, 12, "log" );
    bay.spawn_item( 3, 13, "log" );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_7( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_58", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.ter_set( 3, 8, t_conveyor );
    bay.ter_set( 3, 7, t_conveyor );
    bay.ter_set( 3, 6, t_conveyor );
    bay.ter_set( 3, 5, t_conveyor );
    bay.furn_set( 8, 6, f_rack );
    bay.furn_set( 8, 7, f_rack );
    bay.draw_square_ter( t_sidewalk, 5, 17, 7, 19 );
    bay.ter_set( 6, 18, t_water_pump );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_56", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_palisade, 16, 17, 16, 23 );
    bay.draw_square_ter( t_palisade, 16, 14, 19, 14 );
    bay.draw_square_ter( t_palisade, 19, 11, 19, 13 );
    //bay.ter_set(16, 5, t_palisade);
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_65", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_dirt, 0, 4, 12, 18 );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_8( mission *miss )
{
    //Finish Sawmill
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_58", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.spawn_item( 3, 2, "log" );
    bay.spawn_item( 3, 3, "log" );
    bay.spawn_item( 3, 4, "log" );
    bay.spawn_item( 1, 3, "log", rng( 1, 5 ) );
    bay.spawn_item( 0, 3, "log", rng( 1, 5 ) );
    bay.draw_square_ter( t_machinery_old, 3, 9, 3, 10 );
    bay.ter_set( 3, 11, t_machinery_heavy );
    bay.spawn_item( 3, 12, "2x4", rng( 1, 10 ) );
    bay.place_npc( 4, 10, string_id<npc_template>( "ranch__woodcutter_2" ) );
    bay.save();

    //Finish west wall
    site = mission_util::target_om_ter_random( "ranch_camp_56", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_palisade, 20, 11, 23, 11 );
    bay.draw_square_ter( t_palisade, 23, 7, 23, 10 );
    bay.save();

    //Finish small field to west of barn
    site = mission_util::target_om_ter_random( "ranch_camp_65", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_dirtmound, 1, 5, 2, 17 );
    bay.draw_square_ter( t_dirtmound, 4, 5, 5, 17 );
    bay.draw_square_ter( t_dirtmound, 7, 5, 8, 17 );
    bay.draw_square_ter( t_dirtmound, 10, 5, 11, 17 );
    bay.save();

    //Start Outhouse
    site = mission_util::target_om_ter_random( "ranch_camp_68", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_wall_half, 16, 1, 19, 5 );
    bay.draw_square_ter( t_dirtfloor, 16, 2, 17, 2 );
    bay.draw_square_ter( t_dirtfloor, 16, 4, 17, 4 );
    bay.ter_set( 18, 2, t_pit );
    bay.ter_set( 18, 4, t_pit );
    bay.save();

    //Start Tool shed
    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_wall_half, 13, 17, 18, 21 );
    bay.draw_square_ter( t_dirtfloor, 13, 19, 13, 19 );
    bay.draw_square_ter( t_dirtfloor, 14, 18, 17, 20 );
    bay.draw_square_ter( t_dirt, 10, 23, 12, 23 );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_9( mission *miss )
{
    //Finish Outhouse
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_68", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.ter_set( 16, 2, t_door_c );
    bay.ter_set( 16, 4, t_door_c );
    bay.place_npc( 17, 4, string_id<npc_template>( "ranch_ill_1" ) );
    bay.save();

    //Finish Tool shed
    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.ter_set( 13, 19, t_door_c );
    bay.ter_set( 18, 19, t_window_boarded_noglass );
    bay.draw_line_furn( f_counter, 14, 18, 17, 18 );
    bay.draw_line_furn( f_counter, 14, 20, 17, 20 );
    bay.spawn_item( 14, 18, "hammer_sledge" );

    //Start Clinic
    bay.draw_square_ter( t_wall_half, 2, 3, 9, 10 );
    bay.draw_square_ter( t_wall_half, 10, 5, 12, 9 );
    bay.draw_square_ter( t_wall_half, 13, 3, 20, 10 );
    bay.draw_square_ter( t_dirt, 3, 4, 8, 9 );
    bay.draw_square_ter( t_dirt, 10, 6, 12, 8 );
    bay.draw_square_ter( t_dirt, 14, 4, 19, 9 );
    bay.draw_square_ter( t_dirt, 9, 7, 13, 7 );
    bay.draw_square_ter( t_dirt, 5, 10, 6, 10 );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_10( mission *miss )
{
    //Continue Clinic
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.ter_set( 2, 5, t_window_boarded_noglass );
    bay.ter_set( 2, 8, t_window_boarded_noglass );
    bay.ter_set( 20, 5, t_window_boarded_noglass );
    bay.ter_set( 20, 8, t_window_boarded_noglass );
    bay.spawn_item( 15, 18, "ax" );
    bay.save();

    //Start Chop-Shop
    site = mission_util::target_om_ter_random( "ranch_camp_60", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_wall_half, 0, 9, 16, 18 );
    bay.draw_square_ter( t_dirt, 1, 10, 15, 17 );
    bay.draw_square_ter( t_dirt, 0, 11, 16, 16 );
    bay.draw_square_ter( t_wall_half, 5, 19, 13, 21 );
    bay.draw_square_ter( t_dirt, 6, 19, 8, 20 );
    bay.draw_square_ter( t_dirt, 10, 19, 12, 20 );
    bay.ter_set( 7, 18, t_door_frame );
    bay.ter_set( 11, 21, t_door_frame );
    bay.ter_set( 11, 18, t_door_frame );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_11( mission *miss )
{
    //Continue Clinic
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_floor, 3, 4, 8, 9 );
    bay.draw_square_ter( t_floor, 10, 6, 12, 8 );
    bay.draw_square_ter( t_floor, 14, 4, 19, 9 );
    bay.draw_square_ter( t_floor, 9, 7, 13, 7 );
    bay.draw_square_ter( t_floor, 5, 10, 6, 10 );
    bay.ter_set( 5, 10, t_door_c );
    bay.ter_set( 6, 10, t_door_c );
    bay.draw_square_furn( f_cupboard, 15, 4, 18, 4 );
    bay.draw_square_furn( f_table, 16, 7, 17, 7 );
    bay.place_npc( 5, 6, string_id<npc_template>( "ranch_nurse_1" ) );
    bay.spawn_item( 16, 18, "hoe" );
    bay.save();

    //Continue Chop-Shop
    site = mission_util::target_om_ter_random( "ranch_camp_60", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.translate( t_door_frame, t_door_c );
    bay.save();

    //Start adding scrap vehicles
    site = mission_util::target_om_ter_random( "ranch_camp_61", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.add_vehicle( vproto_id( "car" ), 1, 20, 270 );
    bay.add_vehicle( vproto_id( "flatbed_truck" ), 10, 23, 270 );

    bay.add_vehicle( vproto_id( "car_sports" ), 3, 9, 90 );
    bay.add_vehicle( vproto_id( "cube_van_cheap" ), 10, 10, 90 );
    bay.save();

    //Start expanding vehicle wall
    site = mission_util::target_om_ter_random( "ranch_camp_69", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.add_vehicle( vproto_id( "car_chassis" ), 3, 14, 0 );
    bay.add_vehicle( vproto_id( "pickup" ), 8, 15, 0 );
    bay.add_vehicle( vproto_id( "schoolbus" ), 22, 13, 135 );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_12( mission *miss )
{
    //Finish Chop-Shop
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_60", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_dirtfloor, 1, 10, 15, 17 );
    bay.draw_square_ter( t_dirtfloor, 0, 11, 16, 16 );
    bay.draw_square_ter( t_dirtfloor, 6, 19, 8, 20 );
    bay.draw_square_ter( t_dirtfloor, 10, 19, 12, 20 );
    bay.add_vehicle( vproto_id( "armored_car" ), 7, 15, 180 );
    bay.draw_square_furn( f_rack, 3, 10, 5, 10 );
    bay.draw_square_furn( f_counter, 9, 10, 12, 10 );
    bay.draw_square_furn( f_rack, 10, 19, 10, 20 );
    bay.draw_square_furn( f_makeshift_bed, 8, 19, 8, 20 );
    bay.draw_square_furn( f_rack, 6, 19, 6, 20 );
    bay.place_npc( 13, 12, string_id<npc_template>( "ranch_scrapper_1" ) );
    bay.save();

    //Start Junk Shop
    site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_wall_half, 0, 9, 6, 14 );
    bay.draw_square_ter( t_wall_half, 6, 10, 16, 18 );
    bay.draw_square_ter( t_wall_half, 8, 19, 14, 23 );
    bay.draw_square_ter( t_dirt, 1, 10, 5, 13 );
    bay.draw_square_ter( t_dirt, 7, 11, 15, 17 );
    bay.draw_square_ter( t_dirt, 9, 18, 13, 22 );
    bay.ter_set( 6, 12, t_dirt );
    bay.ter_set( 6, 16, t_dirt );
    bay.ter_set( 14, 21, t_dirt );
    bay.save();

    //Continue expanding vehicle wall
    site = mission_util::target_om_ter_random( "ranch_camp_69", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.add_vehicle( vproto_id( "cube_van" ), 13, 15, 180 );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_13( mission *miss )
{
    //Continue Junk Shop
    tripoint site = mission_util::target_om_ter( "ranch_camp_49", 1, miss, false );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.ter_set( 6, 12, t_dirtfloor );
    bay.ter_set( 6, 16, t_door_frame );
    bay.ter_set( 14, 21, t_door_frame );
    bay.draw_square_ter( t_window_frame, 0, 11, 0, 12 );
    bay.draw_square_ter( t_window_frame, 10, 10, 12, 10 );
    bay.draw_square_ter( t_window_frame, 16, 13, 16, 15 );
    bay.ter_set( 8, 21, t_window_frame );
    bay.save();

    //Continue expanding vehicle wall
    site = mission_util::target_om_ter( "ranch_camp_70", 1, miss, false );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.add_vehicle( vproto_id( "car_mini" ), 8, 3, 45 );
    bay.save();

    site = mission_util::target_om_ter( "ranch_camp_66", 1, miss, false );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.place_npc( 5, 3, string_id<npc_template>( "ranch_barber" ) );
    bay.save();

    mission_util::target_om_ter( "ranch_camp_67", 1, miss, false );
}

void mission_start::ranch_construct_14( mission *miss )
{
    //Finish Junk Shop
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_door_frame, t_door_c );
    bay.translate( t_window_frame, t_window_boarded_noglass );
    bay.draw_square_ter( t_dirtfloor, 1, 10, 5, 13 );
    bay.draw_square_ter( t_dirtfloor, 7, 11, 15, 17 );
    bay.draw_square_ter( t_dirtfloor, 9, 18, 13, 22 );
    bay.draw_square_furn( f_counter, 11, 20, 11, 22 );
    bay.draw_square_furn( f_rack, 15, 11, 15, 17 );
    bay.draw_square_furn( f_rack, 1, 10, 5, 10 );
    bay.draw_square_furn( f_rack, 1, 13, 5, 13 );
    bay.draw_square_furn( f_counter, 9, 13, 10, 16 );
    bay.draw_square_furn( f_counter, 12, 13, 13, 16 );
    bay.place_npc( 10, 21, string_id<npc_template>( "ranch_scavenger_1" ) );
    bay.save();

    //Start Bar
    site = mission_util::target_om_ter_random( "ranch_camp_51", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_wall_half, 0, 18, 14, 23 );
    bay.draw_square_ter( t_dirt, 1, 19, 13, 23 );
    bay.draw_square_ter( t_wall_half, 14, 18, 19, 22 );
    bay.draw_square_ter( t_dirt, 15, 19, 18, 21 );
    bay.draw_square_ter( t_wall_half, 7, 15, 10, 18 );
    bay.draw_square_ter( t_dirt, 8, 16, 9, 17 );
    bay.draw_square_ter( t_wall_half, 10, 15, 14, 18 );
    bay.draw_square_ter( t_dirt, 11, 16, 13, 17 );
    bay.ter_set( 8, 18, t_door_frame );
    bay.ter_set( 12, 18, t_door_frame );
    bay.ter_set( 14, 20, t_door_frame );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_60", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_wall_half, 0, 0, 14, 1 );
    bay.draw_square_ter( t_wall_half, 3, 1, 11, 4 );
    bay.draw_square_ter( t_dirt, 1, 0, 13, 0 );
    bay.draw_square_ter( t_dirt, 4, 1, 10, 3 );
    bay.draw_square_ter( t_door_frame, 3, 2, 3, 3 );
    bay.save();

    //Continue expanding vehicle wall
    site = mission_util::target_om_ter_random( "ranch_camp_61", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.add_vehicle( vproto_id( "ambulance" ), 14, 4, 90 );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_15( mission *miss )
{
    //Continue Bar
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_51", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.draw_square_ter( t_window_frame, 0, 21, 0, 22 );
    bay.draw_square_ter( t_window_frame, 3, 18, 4, 18 );
    bay.save();
    site = mission_util::target_om_ter_random( "ranch_camp_60", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.ter_set( 7, 4, t_window_frame );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_construct_16( mission *miss )
{
    //Finish Bar
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_51", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_window_frame, t_window_boarded_noglass );
    bay.translate( t_door_frame, t_door_c );
    bay.draw_square_ter( t_dirtfloor, 1, 19, 13, 23 );
    bay.draw_square_ter( t_dirtfloor, 15, 19, 18, 21 );
    bay.draw_square_ter( t_dirtfloor, 8, 16, 9, 17 );
    bay.draw_square_ter( t_dirtfloor, 11, 16, 13, 17 );
    bay.draw_square_furn( f_counter, 11, 20, 11, 23 );
    bay.draw_square_furn( f_chair, 1, 20, 2, 23 );
    bay.draw_square_furn( f_table, 1, 21, 2, 22 );
    bay.draw_square_furn( f_chair, 5, 23, 7, 23 );
    bay.draw_square_furn( f_table, 6, 23, 6, 23 );
    bay.draw_square_furn( f_chair, 5, 20, 7, 20 );
    bay.draw_square_furn( f_table, 6, 20, 6, 20 );
    bay.draw_square_furn( f_rack, 13, 21, 13, 22 );
    bay.draw_square_furn( f_rack, 11, 16, 11, 17 );
    bay.draw_square_furn( f_wood_keg, 16, 19, 17, 19 );
    bay.draw_square_furn( f_fvat_empty, 16, 21, 17, 21 );
    //Do a check to prevent duplicate NPCs in the last mission of each version
    const std::vector<std::shared_ptr<npc>> all_npcs = overmap_buffer.get_npcs_near( site.x * 2,
                                         site.y * 2, site.z, 3 );
    bool already_has = false;
    unsigned int a = -1;
    for( auto &elem : all_npcs ) {
        if( elem->name.find( ", Bartender" ) != a ) {
            already_has = true;
        }
    }
    if( !already_has ) {
        bay.place_npc( 12, 22, string_id<npc_template>( "ranch_bartender" ) );
        bay.place_npc( 7, 20, string_id<npc_template>( "scavenger_merc" ) );
    }
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_60", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_window_frame, t_window_boarded_noglass );
    bay.translate( t_door_frame, t_door_c );
    bay.draw_square_ter( t_dirtfloor, 1, 0, 13, 0 );
    bay.draw_square_ter( t_dirtfloor, 4, 1, 10, 3 );
    bay.furn_set( 11, 0, f_counter );
    bay.save();

    //Start Greenhouse
    site = mission_util::target_om_ter_random( "ranch_camp_52", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_underbrush, t_dirt );
    bay.draw_square_ter( t_dirt, 1, 9, 13, 23 );
    bay.draw_square_ter( t_wall_half, 2, 10, 12, 22 );
    bay.draw_square_ter( t_door_frame, 7, 10, 7, 22 );
    bay.draw_square_ter( t_dirt, 3, 11, 11, 21 );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_67", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_nurse_1( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_furn( f_rack, 16, 9, 17, 9 );
    bay.spawn_item( 16, 9, "bandages", rng( 1, 3 ) );
    bay.spawn_item( 17, 9, "aspirin", rng( 1, 2 ) );
    bay.save();
}

void mission_start::ranch_nurse_2( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_furn( f_counter, 3, 7, 5, 7 );
    bay.draw_square_furn( f_rack, 8, 4, 8, 5 );
    bay.spawn_item( 8, 4, "manual_first_aid" );
    bay.save();
}

void mission_start::ranch_nurse_3( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_dirt, 2, 16, 9, 23 );
    bay.draw_square_ter( t_dirt, 13, 16, 20, 23 );
    bay.draw_square_ter( t_dirt, 10, 17, 12, 23 );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_dirt, 2, 0, 20, 2 );
    bay.draw_square_ter( t_dirt, 10, 3, 12, 4 );
    bay.save();
}

void mission_start::ranch_nurse_4( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_wall_half, 2, 16, 9, 23 );
    bay.draw_square_ter( t_dirt, 3, 17, 8, 22 );
    bay.draw_square_ter( t_wall_half, 13, 16, 20, 23 );
    bay.draw_square_ter( t_dirt, 14, 17, 19, 22 );
    bay.draw_square_ter( t_wall_half, 10, 17, 12, 23 );
    bay.draw_square_ter( t_dirt, 10, 18, 12, 23 );
    bay.ter_set( 9, 19, t_door_frame );
    bay.ter_set( 13, 19, t_door_frame );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_wall_half, 4, 0, 18, 2 );
    bay.draw_square_ter( t_wall_half, 10, 3, 12, 4 );
    bay.draw_square_ter( t_dirt, 5, 0, 8, 2 );
    bay.draw_square_ter( t_dirt, 10, 0, 12, 4 );
    bay.draw_square_ter( t_dirt, 14, 0, 17, 2 );
    bay.ter_set( 9, 1, t_door_frame );
    bay.ter_set( 13, 1, t_door_frame );
    bay.save();
}

void mission_start::ranch_nurse_5( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.ter_set( 2, 21, t_window_frame );
    bay.ter_set( 2, 18, t_window_frame );
    bay.ter_set( 20, 18, t_window_frame );
    bay.ter_set( 20, 21, t_window_frame );
    bay.ter_set( 11, 17, t_window_frame );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_wall_half, t_wall_wood );
    bay.draw_square_ter( t_dirt, 10, 0, 12, 4 );
    bay.save();
}

void mission_start::ranch_nurse_6( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_window_frame, t_window_boarded_noglass );
    bay.translate( t_door_frame, t_door_c );
    bay.draw_square_ter( t_dirtfloor, 3, 17, 8, 22 );
    bay.draw_square_ter( t_dirtfloor, 14, 17, 19, 22 );
    bay.draw_square_ter( t_dirtfloor, 10, 18, 12, 23 );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_door_frame, t_door_c );
    bay.draw_square_ter( t_dirtfloor, 5, 0, 8, 2 );
    bay.draw_square_ter( t_dirtfloor, 10, 0, 12, 4 );
    bay.draw_square_ter( t_dirtfloor, 14, 0, 17, 2 );
    bay.save();
}

void mission_start::ranch_nurse_7( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_dirtfloor, t_floor );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_dirtfloor, t_floor );
    bay.draw_square_ter( t_floor, 10, 5, 12, 5 );
    bay.draw_square_furn( f_rack, 17, 0, 17, 2 );
    bay.save();
}

void mission_start::ranch_nurse_8( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_furn( f_makeshift_bed, 4, 21, 4, 22 );
    bay.draw_square_furn( f_makeshift_bed, 7, 21, 7, 22 );
    bay.draw_square_furn( f_makeshift_bed, 15, 21, 15, 22 );
    bay.draw_square_furn( f_makeshift_bed, 18, 21, 18, 22 );
    bay.draw_square_furn( f_makeshift_bed, 4, 17, 4, 18 );
    bay.draw_square_furn( f_makeshift_bed, 7, 17, 7, 18 );
    bay.draw_square_furn( f_makeshift_bed, 15, 17, 15, 18 );
    bay.draw_square_furn( f_makeshift_bed, 18, 17, 18, 18 );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_dirtfloor, t_floor );
    bay.place_items( "cleaning", 75, 17, 0, 17, 2, true, 0 );
    bay.place_items( "surgery", 75, 15, 4, 18, 4, true, 0 );
    bay.save();
}

void mission_start::ranch_nurse_9( mission *miss )
{
    //Improvements to clinic...
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_50", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.furn_set( 3, 22, f_dresser );
    bay.furn_set( 8, 22, f_dresser );
    bay.furn_set( 14, 22, f_dresser );
    bay.furn_set( 19, 22, f_dresser );
    bay.furn_set( 3, 17, f_dresser );
    bay.furn_set( 8, 17, f_dresser );
    bay.furn_set( 14, 17, f_dresser );
    bay.furn_set( 19, 17, f_dresser );
    bay.place_npc( 16, 19, string_id<npc_template>( "ranch_doctor" ) );
    bay.save();

    mission_util::target_om_ter_random( "ranch_camp_59", 1, miss, false, RANCH_SIZE );
}

void mission_start::ranch_scavenger_1( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_48", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_chainfence, 15, 13, 15, 22 );
    bay.draw_square_ter( t_chainfence, 16, 13, 23, 13 );
    bay.draw_square_ter( t_chainfence, 16, 22, 23, 22 );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.place_items( "mechanics", 65, 9, 13, 10, 16, true, 0 );
    bay.draw_square_ter( t_chainfence, 0, 22, 7, 22 );
    bay.draw_square_ter( t_dirt, 2, 22, 3, 22 );
    bay.spawn_item( 7, 19, "30gal_drum" );
    bay.save();
}

void mission_start::ranch_scavenger_2( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_48", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.add_vehicle( vproto_id( "car_chassis" ), 20, 15, 0 );
    bay.draw_square_ter( t_wall_half, 18, 19, 21, 22 );
    bay.draw_square_ter( t_dirt, 19, 20, 20, 21 );
    bay.ter_set( 19, 19, t_door_frame );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.place_items( "mischw", 65, 12, 13, 13, 16, true, 0 );
    bay.draw_square_ter( t_chaingate_l, 2, 22, 3, 22 );
    bay.spawn_item( 7, 20, "30gal_drum" );
    bay.save();
}

void mission_start::ranch_scavenger_3( mission *miss )
{
    tripoint site = mission_util::target_om_ter_random( "ranch_camp_48", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_door_frame, t_door_locked );
    bay.translate( t_wall_half, t_wall_wood );
    bay.draw_square_ter( t_dirtfloor, 19, 20, 20, 21 );
    bay.spawn_item( 16, 21, "wheel_wide" );
    bay.spawn_item( 17, 21, "wheel_wide" );
    bay.spawn_item( 23, 18, "v8_combustion" );
    bay.furn_set( 23, 17, furn_str_id( "f_arcade_machine" ) );
    bay.ter_set( 23, 16, ter_str_id( "t_machinery_light" ) );
    bay.furn_set( 20, 21, f_woodstove );
    bay.save();

    site = mission_util::target_om_ter_random( "ranch_camp_49", 1, miss, false, RANCH_SIZE );
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.place_items( "mischw", 65, 2, 10, 4, 10, true, 0 );
    bay.place_items( "mischw", 65, 2, 13, 4, 13, true, 0 );
    bay.furn_set( 1, 15, f_fridge );
    bay.spawn_item( 2, 15, "hdframe" );
    bay.furn_set( 3, 15, f_washer );
    bay.save();
}

void mission_start::ranch_bartender_1( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->my_fac->wealth += rng( 500, 2500 );
    p->set_mutation( trait_id( "NPC_BRANDY" ) );

    tripoint site = mission_util::target_om_ter_random( "ranch_camp_51", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_wall_half, 1, 15, 7, 17 );
    bay.draw_square_ter( t_dirt, 2, 16, 6, 17 );
    bay.draw_square_ter( t_wall_half, 0, 8, 14, 15 );
    bay.draw_square_ter( t_dirt, 1, 9, 13, 14 );
    bay.ter_set( 0, 10, t_door_frame );
    bay.ter_set( 0, 11, t_door_frame );
    bay.draw_square_ter( t_dirtfloor, 3, 15, 5, 15 );
    bay.draw_square_ter( t_dirtfloor, 3, 18, 5, 18 );
    //bay.draw_square_ter( t_dirtfloor, 15, 18, 16, 18);

    bay.save();
}

void mission_start::ranch_bartender_2( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->my_fac->wealth += rng( 500, 2500 );
    p->set_mutation( trait_id( "NPC_RUM" ) );

    tripoint site = mission_util::target_om_ter_random( "ranch_camp_51", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_ter( t_wall_half, 14, 10, 19, 15 );
    bay.draw_square_ter( t_dirt, 15, 11, 18, 14 );
    bay.draw_square_ter( t_wall_half, 14, 15, 17, 18 );
    bay.draw_square_ter( t_dirt, 15, 15, 16, 18 );
    bay.translate( t_door_frame, t_door_c );
    bay.translate( t_wall_half, t_wall_wood );
    bay.draw_square_ter( t_window_frame, 0, 13, 0, 13 );
    bay.draw_square_ter( t_window_frame, 4, 8, 5, 8 );
    bay.draw_square_ter( t_window_frame, 9, 8, 10, 8 );
    bay.draw_square_ter( t_window_frame, 19, 12, 19, 12 );
    bay.furn_set( 18, 19, f_wood_keg );
    bay.furn_set( 16, 19, f_null );
    bay.save();
}

void mission_start::ranch_bartender_3( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->my_fac->wealth += rng( 500, 2500 );
    p->set_mutation( trait_id( "NPC_WHISKEY" ) );

    tripoint site = mission_util::target_om_ter_random( "ranch_camp_51", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.translate( t_window_frame, t_window_boarded_noglass );
    bay.draw_square_ter( t_dirtfloor, 15, 11, 18, 14 );
    bay.draw_square_ter( t_dirtfloor, 15, 15, 16, 18 );
    bay.draw_square_ter( t_dirtfloor, 1, 9, 13, 14 );
    bay.draw_square_ter( t_dirtfloor, 15, 11, 18, 14 );
    bay.save();
}

void mission_start::ranch_bartender_4( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    p->my_fac->wealth += rng( 500, 2500 );

    tripoint site = mission_util::target_om_ter_random( "ranch_camp_51", 1, miss, false, RANCH_SIZE );
    tinymap bay;
    bay.load( site.x * 2, site.y * 2, site.z, false );
    bay.draw_square_furn( f_standing_tank, 16, 11, 17, 11 );
    bay.draw_square_furn( f_fvat_empty, 17, 14, 18, 14 );
    bay.draw_square_furn( f_standing_tank, 13, 23, 13, 23 );
    bay.draw_square_furn( f_chair, 4, 10, 6, 10 );
    bay.draw_square_furn( f_table, 5, 10, 5, 10 );
    bay.draw_square_furn( f_chair, 5, 13, 7, 13 );
    bay.draw_square_furn( f_table, 6, 13, 6, 13 );

    bay.draw_square_furn( f_chair, 10, 10, 11, 10 );
    bay.draw_square_furn( f_table, 10, 11, 11, 12 );
    bay.draw_square_furn( f_chair, 10, 13, 11, 13 );
    bay.draw_square_furn( f_chair, 9, 11, 9, 12 );
    bay.draw_square_furn( f_chair, 12, 11, 12, 12 );
    bay.save();
}

void mission_start::place_book( mission * )
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

void reveal_route( mission *miss, const tripoint &destination )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
        return;
    }

    const tripoint source = g->u.global_omt_location();

    const tripoint source_road = overmap_buffer.find_closest( source, "road", 3, false );
    const tripoint dest_road = overmap_buffer.find_closest( destination, "road", 3, false );

    if( overmap_buffer.reveal_route( source_road, dest_road ) ) {
        add_msg( _( "%s also marks the road that leads to it..." ), p->name );
    }
}

void reveal_target( mission *miss, const std::string &omter_id )
{
    const npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "couldn't find an NPC!" );
        return;
    }

    const tripoint destination = reveal_destination( omter_id );
    if( destination != overmap::invalid_tripoint ) {
        const oter_id oter = overmap_buffer.ter( destination );
        add_msg( _( "%s has marked the only %s known to them on your map." ), p->name, oter->get_name() );
        miss->set_target( destination );
        if( one_in( 3 ) ) {
            reveal_route( miss, destination );
        }
    }
}

void reveal_any_target( mission *miss, const std::vector<std::string> &omter_ids )
{
    reveal_target( miss, random_entry( omter_ids ) );
}

void mission_start::reveal_refugee_center( mission *miss )
{
    mission_target_params t;
    t.search_origin = g->u.global_omt_location();
    t.overmap_terrain_subtype = "evac_center_18";
    t.overmap_special = overmap_special_id( "evac_center" );
    t.mission_pointer = miss;
    t.search_range = OMAPX * 5;
    t.reveal_radius = 3;

    const cata::optional<tripoint> target_pos = assign_mission_target( t );

    if( !target_pos ) {
        add_msg( _( "You don't know where the address could be..." ) );
        return;
    }

    const tripoint source_road = overmap_buffer.find_closest( *t.search_origin, "road", 3, false );
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
        compmap.load( om_place.x * 2, om_place.y * 2, om_place.z, false );

        tripoint comppoint = find_potential_computer_point( compmap, om_place.z );

        computer *tmpcomp = compmap.add_computer( comppoint, _( comp_name.c_str() ), security );
        tmpcomp->mission_id = miss->get_id();
        tmpcomp->add_option( _( download_name.c_str() ), COMPACT_DOWNLOAD_SOFTWARE, security );
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

    create_lab_consoles( miss, place, "lab", 2, "Workstation", "Download Memory Contents" );

    // Target the lab entrance.
    const tripoint target = target_closest_lab_entrance( place, 2, miss );
    reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}

void mission_start::create_hidden_lab_console( mission *miss )
{
    // Pick a hidden lab entrance.
    tripoint loc = g->u.global_omt_location();
    loc.z = -1;
    tripoint place = mission_util::target_om_ter_random( "basement_hidden_lab_stairs", -1, miss, false,
                     0, loc );
    place.z = -2;  // then go down 1 z-level to place consoles.

    create_lab_consoles( miss, place, "lab", 3, "Workstation", "Download Encryption Routines" );

    // Target the lab entrance.
    const tripoint target = target_closest_lab_entrance( place, 2, miss );
    reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}

void mission_start::create_ice_lab_console( mission *miss )
{
    // Pick an ice lab with spaces on z = -4.
    tripoint loc = g->u.global_omt_location();
    loc.z = -4;
    const tripoint place = overmap_buffer.find_closest( loc, "ice_lab", 0, false );

    create_lab_consoles( miss, place, "ice_lab", 3, "Durable Storage Archive", "Download Archives" );

    // Target the lab entrance.
    const tripoint target = target_closest_lab_entrance( place, 2, miss );
    reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}

void mission_start::reveal_lab_train_depot( mission *miss )
{
    // Find and prepare lab location.
    tripoint loc = g->u.global_omt_location();
    loc.z = -4;  // tunnels are at z = -4
    const tripoint place = overmap_buffer.find_closest( loc, "lab_train_depot", 0, false );

    tinymap compmap;
    compmap.load( place.x * 2, place.y * 2, place.z, false );
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
    const tripoint target = target_closest_lab_entrance( place, 2, miss );
    reveal_road( g->u.global_omt_location(), target, overmap_buffer );
}

void mission_util::set_reveal( const std::string &terrain,
                               std::vector<std::function<void( mission *miss )>> &funcs )
{
    const auto mission_func = [ terrain ]( mission * miss ) {
        reveal_target( miss, terrain );
    };
    funcs.emplace_back( mission_func );
}

void mission_util::set_reveal_any( JsonArray &ja,
                                   std::vector<std::function<void( mission *miss )>> &funcs )
{
    std::vector<std::string> terrains;
    while( ja.has_more() ) {
        std::string terrain = ja.next_string();
        terrains.push_back( terrain );
    }
    const auto mission_func = [ terrains ]( mission * miss ) {
        reveal_any_target( miss, terrains );
    };
    funcs.emplace_back( mission_func );
}

void mission_util::set_assign_om_target( JsonObject &jo,
        std::vector<std::function<void( mission *miss )>> &funcs )
{
    if( !jo.has_string( "om_terrain" ) ) {
        jo.throw_error( "'om_terrain' is required for assign_mission_target" );
    }
    mission_target_params p;
    p.overmap_terrain_subtype = jo.get_string( "om_terrain" );
    if( jo.has_string( "om_terrain_replace" ) ) {
        p.replaceable_overmap_terrain_subtype = jo.get_string( "om_terrain_replace" );
    }
    if( jo.has_string( "om_special" ) ) {
        p.overmap_special = overmap_special_id( jo.get_string( "om_special" ) );
    }
    if( jo.has_int( "reveal_radius" ) ) {
        p.reveal_radius = std::max( 1, jo.get_int( "reveal_radius" ) );
    }
    if( jo.has_bool( "must_see" ) ) {
        p.must_see = jo.get_bool( "must_see" );
    }
    if( jo.has_bool( "random" ) ) {
        p.random = jo.get_bool( "random" );
    }
    if( jo.has_int( "search_range" ) ) {
        p.search_range = std::max( 1, jo.get_int( "search_range" ) );
    }
    cata::optional<int> z;
    if( jo.has_int( "z" ) ) {
        z = jo.get_int( "z" );
    }
    const auto mission_func = [p, z]( mission * miss ) {
        mission_target_params mtp = p;
        mtp.mission_pointer = miss;
        if( z ) {
            const tripoint loc = g->u.global_omt_location();
            mtp.search_origin = tripoint( loc.x, loc.y, *z );
        }
        assign_mission_target( mtp );
    };
    funcs.emplace_back( mission_func );
}

bool mission_util::set_update_mapgen( JsonObject &jo,
                                      std::vector<std::function<void( mission *miss )>> &funcs )
{
    bool defer = false;
    mapgen_update_func update_map = add_mapgen_update_func( jo, defer );
    if( defer ) {
        return false;
    }

    if( jo.has_string( "om_special" ) && jo.has_string( "om_terrain" ) ) {
        const std::string om_terrain = jo.get_string( "om_terrain" );
        const auto mission_func = [update_map, om_terrain]( mission * miss ) {
            tripoint update_pos3 = mission_util::reveal_om_ter( om_terrain, 1, false );
            update_map( update_pos3, miss );
        };
        funcs.emplace_back( mission_func );
    } else {
        const auto mission_func = [update_map]( mission * miss ) {
            tripoint update_pos3 = miss->get_target();
            update_map( update_pos3, miss );
        };
        funcs.emplace_back( mission_func );
    }
    return true;
}

bool mission_util::load_funcs( JsonObject jo,
                               std::vector<std::function<void( mission *miss )>> &funcs )
{
    if( jo.has_string( "reveal_om_ter" ) ) {
        const std::string target_terrain = jo.get_string( "reveal_om_ter" );
        set_reveal( target_terrain, funcs );
    } else if( jo.has_array( "reveal_om_ter" ) ) {
        JsonArray target_terrain = jo.get_array( "reveal_om_ter" );
        set_reveal_any( target_terrain, funcs );
    } else if( jo.has_object( "assign_mission_target" ) ) {
        JsonObject mission_target = jo.get_object( "assign_mission_target" );
        set_assign_om_target( mission_target, funcs );
    }

    if( jo.has_object( "update_mapgen" ) ) {
        JsonObject update_mapgen = jo.get_object( "update_mapgen" );
        if( !set_update_mapgen( update_mapgen, funcs ) ) {
            return false;
        }
    } else if( jo.has_array( "update_mapgen" ) ) {
        JsonArray mapgen_array = jo.get_array( "update_mapgen" );
        while( mapgen_array.has_more() ) {
            JsonObject update_mapgen = mapgen_array.next_object();
            if( !set_update_mapgen( update_mapgen, funcs ) ) {
                return false;
            }
        }
    }

    return true;
}

bool mission_type::parse_funcs( JsonObject &jo, std::function<void( mission * )> &phase_func )
{
    std::vector<std::function<void( mission *miss )>> funcs;
    if( !mission_util::load_funcs( jo, funcs ) ) {
        return false;
    }

    /* this is a kind of gross hijack of the dialogue responses effect system, but I don't want to
     * write that code in two places so here it goes.
     */
    talk_effect_t talk_effects;
    talk_effects.load_effect( jo );
    phase_func = [ funcs, talk_effects ]( mission * miss ) {
        ::dialogue d;
        d.beta = g->find_npc( miss->get_npc_id() );
        if( d.beta == nullptr ) {
            standard_npc default_npc( "Default" );
            d.beta = &default_npc;
        }
        d.alpha = &g->u;
        for( const talk_effect_fun_t &effect : talk_effects.effects ) {
            effect( d );
        }
        for( auto &mission_function : funcs ) {
            mission_function( miss );
        }
    };
    return true;
}
