#include "mission.h" // IWYU pragma: associated

#include <memory>
#include <new>
#include <optional>
#include <vector>

#include "avatar.h"
#include "character.h"
#include "computer.h"
#include "coordinates.h"
#include "debug.h"
#include "enum_traits.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "npc.h"
#include "npc_class.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"

static const furn_str_id furn_f_bed( "f_bed" );
static const furn_str_id furn_f_console( "f_console" );
static const furn_str_id furn_f_console_broken( "f_console_broken" );
static const furn_str_id furn_f_dresser( "f_dresser" );

static const itype_id itype_software_hacking( "software_hacking" );
static const itype_id itype_software_math( "software_math" );
static const itype_id itype_software_medical( "software_medical" );
static const itype_id itype_software_useless( "software_useless" );

static const mission_type_id
mission_MISSION_GET_ZOMBIE_BLOOD_ANAL( "MISSION_GET_ZOMBIE_BLOOD_ANAL" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_wall_metal( "t_wall_metal" );

/* These functions are responsible for making changes to the game at the moment
 * the mission is accepted by the player.  They are also responsible for
 * updating *miss with the target and any other important information.
 */

void mission_start::standard( mission * )
{
}

void mission_start::kill_nemesis( mission * )
{
    // Pick an area for the nemesis to spawn

    // Force z = 0 for overmapbuffer::find_random. (spawning on a rooftop is valid!)
    const tripoint_abs_omt center = { get_player_character().global_omt_location().xy(), 0 };
    tripoint_abs_omt site = overmap::invalid_tripoint;

    static const std::array<float, 3> attempts_multipliers {1.0f, 1.5f, 2.f};

    size_t attempt = 0;
    do {
        if( attempt++ >= attempts_multipliers.size() ) {
            debugmsg( "Failed adding a nemesis mission" );
            return;
        }
        int range = rng( 40, 80 ) * attempts_multipliers[attempt - 1];
        site = overmap_buffer.find_random( center, "field", range, false );
    } while( site == overmap::invalid_tripoint );
    overmap_buffer.add_nemesis( site );
}

/*
 * Find a location to place a computer.  In order, prefer:
 * 1) Broken consoles.
 * 2) Corners or coords adjacent to a bed/dresser? (this logic may be flawed, dates from Whales in 2011)
 * 3) A spot near the center of the tile that is not a console
 * 4) A random spot near the center of the tile.
 */
static tripoint find_potential_computer_point( const tinymap &compmap )
{
    constexpr int rng_x_min = 10;
    constexpr int rng_x_max = SEEX * 2 - 11;
    constexpr int rng_y_min = 10;
    constexpr int rng_y_max = SEEY * 2 - 11;
    static_assert( rng_x_min <= rng_x_max && rng_y_min <= rng_y_max, "invalid randomization range" );
    std::vector<tripoint> broken;
    std::vector<tripoint> potential;
    std::vector<tripoint> last_resort;
    for( const tripoint &p : compmap.points_on_zlevel() ) {
        if( compmap.furn( p ) == furn_f_console_broken ) {
            broken.emplace_back( p );
        } else if( broken.empty() && compmap.ter( p ) == ter_t_floor &&
                   compmap.furn( p ) == furn_str_id::NULL_ID() ) {
            for( const tripoint &p2 : compmap.points_in_radius( p, 1 ) ) {
                if( compmap.furn( p2 ) == furn_f_bed || compmap.furn( p2 ) == furn_f_dresser ) {
                    potential.emplace_back( p );
                    break;
                }
            }
            int wall = 0;
            for( const tripoint &p2 : compmap.points_in_radius( p, 1 ) ) {
                if( compmap.has_flag_ter( ter_furn_flag::TFLAG_WALL, p2 ) ) {
                    wall++;
                }
            }
            if( wall == 5 ) {
                if( compmap.is_last_ter_wall( true, p.xy(), point( SEEX * 2, SEEY * 2 ), direction::NORTH ) &&
                    compmap.is_last_ter_wall( true, p.xy(), point( SEEX * 2, SEEY * 2 ), direction::SOUTH ) &&
                    compmap.is_last_ter_wall( true, p.xy(), point( SEEX * 2, SEEY * 2 ), direction::WEST ) &&
                    compmap.is_last_ter_wall( true, p.xy(), point( SEEX * 2, SEEY * 2 ), direction::EAST ) ) {
                    potential.emplace_back( p );
                }
            }
        } else if( broken.empty() && potential.empty() && p.x >= rng_x_min && p.x <= rng_x_max
                   && p.y >= rng_y_min && p.y <= rng_y_max && compmap.furn( p ) != furn_f_console ) {
            last_resort.emplace_back( p );
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
    const tripoint fallback( rng( rng_x_min, rng_x_max ), rng( rng_y_min, rng_y_max ),
                             compmap.get_abs_sub().z() );
    return random_entry( *used, fallback );
}

void mission_start::place_npc_software( mission *miss )
{
    npc *dev = g->find_npc( miss->npc_id );
    if( dev == nullptr ) {
        debugmsg( "Couldn't find NPC!  %d", miss->npc_id.get_value() );
        return;
    }
    get_player_character().i_add( item( "usb_drive", calendar::turn_zero ) );
    add_msg( _( "%s gave you a USB drive." ), dev->get_name() );

    std::string type = "house";

    if( dev->myclass == NC_HACKER ) {
        miss->item_id = itype_software_hacking;
    } else if( dev->myclass == NC_DOCTOR ) {
        miss->item_id = itype_software_medical;
        static const std::set<std::string> pharmacies = { "s_pharm", "s_pharm_1" };
        type = random_entry( pharmacies );
        miss->follow_up = mission_MISSION_GET_ZOMBIE_BLOOD_ANAL;
    } else if( dev->myclass == NC_SCIENTIST ) {
        miss->item_id = itype_software_math;
    } else {
        miss->item_id = itype_software_useless;
    }

    tripoint_abs_omt place;
    if( type == "house" ) {
        place = mission_util::random_house_in_closest_city();
    } else {
        place = overmap_buffer.find_closest( dev->global_omt_location(), type, 0, false );
    }
    miss->target = place;
    overmap_buffer.reveal( place, 6 );

    tinymap compmap;
    compmap.load( place, false );
    tripoint comppoint;

    oter_id oter = overmap_buffer.ter( place );
    if( is_ot_match( "house", oter, ot_match_type::prefix ) ||
        is_ot_match( "s_pharm", oter, ot_match_type::prefix ) || oter.id().is_empty() ) {
        comppoint = find_potential_computer_point( compmap );
    }

    compmap.i_clear( comppoint );
    compmap.furn_set( comppoint, furn_f_console );
    computer *tmpcomp = compmap.add_computer( comppoint, string_format( _( "%s's Terminal" ),
                        dev->get_name() ), 0 );
    tmpcomp->set_mission( miss->get_id() );
    tmpcomp->add_option( _( "Download Software" ), COMPACT_DOWNLOAD_SOFTWARE, 0 );
    compmap.save();
}

void mission_start::place_deposit_box( mission *miss )
{
    npc *p = g->find_npc( miss->npc_id );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->npc_id.get_value() );
        return;
    }
    // Npc joins you
    p->set_attitude( NPCATT_FOLLOW );
    tripoint_abs_omt site =
        overmap_buffer.find_closest( p->global_omt_location(), "bank", 0, false );
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
    compmap.load( site, false );
    std::vector<tripoint> valid;
    for( const tripoint &p : compmap.points_on_zlevel() ) {
        if( compmap.ter( p ) == ter_t_floor ) {
            for( const tripoint &p2 : compmap.points_in_radius( p, 1 ) ) {
                if( compmap.ter( p2 ) == ter_t_wall_metal ) {
                    valid.push_back( p );
                    break;
                }
            }
        }
    }
    const tripoint fallback( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), site.z() );
    const tripoint comppoint = random_entry( valid, fallback );
    compmap.spawn_item( comppoint, "safe_box" );
    compmap.save();
}

void mission_start::find_safety( mission *miss )
{
    const tripoint_abs_omt place = get_player_character().global_omt_location();
    for( int radius = 0; radius <= 20; radius++ ) {
        for( int dist = 0 - radius; dist <= radius; dist++ ) {
            int offset = rng( 0, 3 ); // Randomizes the direction we check first
            for( int i = 0; i <= 3; i++ ) { // Which direction?
                tripoint_abs_omt check = place;
                switch( ( offset + i ) % 4 ) {
                    case 0:
                        check.x() += dist;
                        check.y() -= radius;
                        break;
                    case 1:
                        check.x() += dist;
                        check.y() += radius;
                        break;
                    case 2:
                        check.y() += dist;
                        check.x() -= radius;
                        break;
                    case 3:
                        check.y() += dist;
                        check.x() += radius;
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

void mission_start::place_book( mission * )
{
}

void mission_start::reveal_refugee_center( mission *miss )
{
    mission_target_params t;
    str_or_var overmap_terrain;
    overmap_terrain.str_val = "refctr_S3e";
    t.overmap_terrain = overmap_terrain;
    str_or_var overmap_special;
    overmap_special.str_val = "evac_center";
    t.overmap_special = overmap_special;
    t.mission_pointer = miss;
    dbl_or_var search_range;
    search_range.min.dbl_val = 0;
    t.search_range = search_range;
    dbl_or_var reveal_radius;
    reveal_radius.min.dbl_val = 1;
    t.reveal_radius = reveal_radius;
    dbl_or_var min_distance;
    min_distance.min.dbl_val = 0;
    t.min_distance = min_distance;

    dialogue d( get_talker_for( get_avatar() ), nullptr );
    std::optional<tripoint_abs_omt> target_pos = mission_util::assign_mission_target( t, d );

    if( !target_pos ) {
        add_msg( _( "You don't know where the address could be…" ) );
        return;
    }

    const tripoint_abs_omt source_road = overmap_buffer.find_closest(
            get_player_character().global_omt_location(), "road",
            3, false );
    const tripoint_abs_omt dest_road = overmap_buffer.find_closest( *target_pos, "road", 3, false );

    if( overmap_buffer.reveal_route( source_road, dest_road, 1, true ) ) {
        //reset the mission target to the refugee center entrance and reveal path from the road
        str_or_var overmap_terrain;
        overmap_terrain.str_val = "refctr_S3e";
        t.overmap_terrain = overmap_terrain;
        dbl_or_var reveal_radius;
        reveal_radius.min.dbl_val = 3;
        t.reveal_radius = reveal_radius;
        target_pos = mission_util::assign_mission_target( t, d );
        const tripoint_abs_omt dest_refugee_center = overmap_buffer.find_closest( *target_pos,
                "evac_center_18", 1, false );
        overmap_buffer.reveal_route( dest_road, dest_refugee_center, 1, false );

        add_msg( _( "You mark the refugee center and the road that leads to it…" ) );
    } else {
        add_msg( _( "You mark the refugee center, but you have no idea how to get there by road…" ) );
    }
}

// Creates multiple lab consoles near tripoint place, which must have its z-level set to where consoles should go.
void static create_lab_consoles(
    mission *miss, const tripoint_abs_omt &place, const std::string &otype, int security,
    const std::string &comp_name, const std::string &download_name )
{
    // Drop four computers in nearby lab spaces so the player can stumble upon one of them.
    for( int i = 0; i < 4; ++i ) {
        tripoint_abs_omt om_place = mission_util::target_om_ter_random(
                                        otype, -1, miss, false, 4, place );

        tinymap compmap;
        compmap.load( om_place, false );

        tripoint comppoint = find_potential_computer_point( compmap );

        computer *tmpcomp = compmap.add_computer( comppoint, comp_name, security );
        tmpcomp->set_mission( miss->get_id() );
        tmpcomp->add_option( download_name, COMPACT_DOWNLOAD_SOFTWARE, security );
        tmpcomp->add_failure( COMPFAIL_ALARM );
        tmpcomp->add_failure( COMPFAIL_DAMAGE );
        tmpcomp->add_failure( COMPFAIL_MANHACKS );

        compmap.save();
    }
}

void mission_start::create_lab_console( mission *miss )
{
    Character &player_character = get_player_character();
    // Pick a lab that has spaces on z = -1: e.g., in hidden labs.
    tripoint_abs_omt loc = player_character.global_omt_location();
    loc.z() = -1;
    const tripoint_abs_omt place = overmap_buffer.find_closest( loc, "lab", 0, false );

    create_lab_consoles( miss, place, "lab", 2, _( "Workstation" ),
                         _( "Download Memory Contents" ) );

    // Target the lab entrance.
    const tripoint_abs_omt target = mission_util::target_closest_lab_entrance( place, 2, miss );
    mission_util::reveal_road( player_character.global_omt_location(), target, overmap_buffer );
}

void mission_start::create_hidden_lab_console( mission *miss )
{
    Character &player_character = get_player_character();
    // Pick a hidden lab entrance.
    tripoint_abs_omt loc = player_character.global_omt_location();
    loc.z() = -1;
    tripoint_abs_omt place = overmap_buffer.find_closest( loc, "basement_hidden_lab_stairs", 0, false );
    place.z() = -2;  // then go down 1 z-level to place consoles.

    create_lab_consoles( miss, place, "lab", 3, _( "Workstation" ),
                         _( "Download Encryption Routines" ) );

    // Target the lab entrance.
    const tripoint_abs_omt target = mission_util::target_closest_lab_entrance( place, 2, miss );
    mission_util::reveal_road( player_character.global_omt_location(), target, overmap_buffer );
}

void mission_start::create_ice_lab_console( mission *miss )
{
    Character &player_character = get_player_character();
    // Pick an ice lab with spaces on z = -4.
    tripoint_abs_omt loc = player_character.global_omt_location();
    loc.z() = -4;
    const tripoint_abs_omt place = overmap_buffer.find_closest( loc, "ice_lab", 0, false );

    create_lab_consoles( miss, place, "ice_lab", 3, _( "Durable Storage Archive" ),
                         _( "Download Archives" ) );

    // Target the lab entrance.
    const tripoint_abs_omt target = mission_util::target_closest_lab_entrance( place, 2, miss );
    mission_util::reveal_road( player_character.global_omt_location(), target, overmap_buffer );
}
