#include "map_extras.h"

#include "cellular_automata.h"
#include "debug.h"
#include "field.h"
#include "fungal_effects.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "mapgen_functions.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "trap.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_group.h"
#include "vpart_position.h"
#include "vpart_range.h"

namespace MapExtras
{

static const mongroup_id GROUP_MAYBE_MIL( "GROUP_MAYBE_MIL" );
static const mongroup_id GROUP_FISH( "GROUP_FISH" );

static const mtype_id mon_zombie_tough( "mon_zombie_tough" );
static const mtype_id mon_blank( "mon_blank" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_scientist( "mon_zombie_scientist" );
static const mtype_id mon_chickenbot( "mon_chickenbot" );
static const mtype_id mon_gelatin( "mon_gelatin" );
static const mtype_id mon_flaming_eye( "mon_flaming_eye" );
static const mtype_id mon_gracke( "mon_gracke" );
static const mtype_id mon_kreck( "mon_kreck" );
static const mtype_id mon_mi_go( "mon_mi_go" );
static const mtype_id mon_tankbot( "mon_tankbot" );
static const mtype_id mon_turret( "mon_turret" );
static const mtype_id mon_turret_bmg( "mon_turret_bmg" );
static const mtype_id mon_turret_rifle( "mon_turret_rifle" );
static const mtype_id mon_zombie_spitter( "mon_zombie_spitter" );
static const mtype_id mon_zombie_soldier( "mon_zombie_soldier" );
static const mtype_id mon_zombie_military_pilot( "mon_zombie_military_pilot" );
static const mtype_id mon_zombie_bio_op( "mon_zombie_bio_op" );
static const mtype_id mon_zombie_grenadier( "mon_zombie_grenadier" );
static const mtype_id mon_shia( "mon_shia" );
static const mtype_id mon_spider_web( "mon_spider_web" );
static const mtype_id mon_jabberwock( "mon_jabberwock" );

void mx_null( map &, const tripoint & )
{
    debugmsg( "Tried to generate null map extra." );
}

void mx_helicopter( map &m, const tripoint &abs_sub )
{
    int cx = rng( 6, SEEX * 2 - 7 );
    int cy = rng( 6, SEEY * 2 - 7 );

    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            if( m.veh_at( tripoint( x,  y, abs_sub.z ) ) &&
                m.ter( tripoint( x, y, abs_sub.z ) )->has_flag( TFLAG_DIGGABLE ) ) {
                m.ter_set( tripoint( x, y, abs_sub.z ), t_dirtmound );
            } else {
                if( x >= cx - dice( 1, 5 ) && x <= cx + dice( 1, 5 ) && y >= cy - dice( 1, 5 ) &&
                    y <= cy + dice( 1, 5 ) ) {
                    if( one_in( 7 ) && m.ter( tripoint( x, y, abs_sub.z ) )->has_flag( TFLAG_DIGGABLE ) ) {
                        m.ter_set( tripoint( x, y, abs_sub.z ), t_dirtmound );
                    }
                }
                if( x >= cx - dice( 1, 6 ) && x <= cx + dice( 1, 6 ) && y >= cy - dice( 1, 6 ) &&
                    y <= cy + dice( 1, 6 ) ) {
                    if( !one_in( 5 ) ) {
                        m.make_rubble( tripoint( x,  y, abs_sub.z ), f_wreckage, true );
                        if( m.ter( tripoint( x, y, abs_sub.z ) )->has_flag( TFLAG_DIGGABLE ) ) {
                            m.ter_set( tripoint( x, y, abs_sub.z ), t_dirtmound );
                        }
                    } else if( m.is_bashable( x, y ) ) {
                        m.destroy( tripoint( x,  y, abs_sub.z ), true );
                        if( m.ter( tripoint( x, y, abs_sub.z ) )->has_flag( TFLAG_DIGGABLE ) ) {
                            m.ter_set( tripoint( x, y, abs_sub.z ), t_dirtmound );
                        }
                    }

                } else if( one_in( 4 + ( abs( x - cx ) + ( abs( y -
                                         cy ) ) ) ) ) { // 1 in 10 chance of being wreckage anyway
                    m.make_rubble( tripoint( x,  y, abs_sub.z ), f_wreckage, true );
                    if( !one_in( 3 ) ) {
                        if( m.ter( tripoint( x, y, abs_sub.z ) )->has_flag( TFLAG_DIGGABLE ) ) {
                            m.ter_set( tripoint( x, y, abs_sub.z ), t_dirtmound );
                        }
                    }
                }
            }
        }
    }

    int dir1 = rng( 0, 359 );

    auto crashed_hull = vgroup_id( "crashed_helicopters" )->pick();

    // Create the vehicle so we can rotate it and calculate its bounding box, but don't place it on the map.
    auto veh = std::unique_ptr<vehicle>( new vehicle( crashed_hull, rng( 1, 33 ), 1 ) );

    veh->turn( dir1 );

    bounding_box bbox = veh->get_bounding_box();     // Get the bounding box, centered on mount(0,0)
    int x_length = std::abs( bbox.p2.x -
                             bbox.p1.x );  // Move the wreckage forward/backward half it's length so
    int y_length = std::abs( bbox.p2.y -   // that it spawns more over the center of the debris area
                             bbox.p1.y );

    int x_offset = veh->dir_vec().x * ( x_length / 2 );   // cont.
    int y_offset = veh->dir_vec().y * ( y_length / 2 );

    int x_min = abs( bbox.p1.x ) + 0;
    int y_min = abs( bbox.p1.y ) + 0;

    int x_max = ( SEEX * 2 ) - ( bbox.p2.x + 1 );
    int y_max = ( SEEY * 2 ) - ( bbox.p2.y + 1 );

    int x1 = clamp( cx + x_offset, x_min,
                    x_max ); // Clamp x1 & y1 such that no parts of the vehicle extend
    int y1 = clamp( cy + y_offset, y_min, y_max ); // over the border of the submap.

    vehicle *wreckage = m.add_vehicle( crashed_hull, tripoint( x1, y1, abs_sub.z ), dir1, rng( 1, 33 ),
                                       1 );

    const auto controls_at = []( vehicle * wreckage, const tripoint & pos ) {
        return !wreckage->get_parts_at( pos, "CONTROLS", part_status_flag::any ).empty() ||
               !wreckage->get_parts_at( pos, "CTRL_ELECTRONIC", part_status_flag::any ).empty();
    };

    if( wreckage != nullptr ) {
        const int clowncar_factor = dice( 1, 8 );

        switch( clowncar_factor ) {
            case 1:
            case 2:
            case 3: // Full clown car
                for( const vpart_reference &vp : wreckage->get_any_parts( VPFLAG_SEATBELT ) ) {
                    const tripoint pos = vp.pos();
                    // Spawn pilots in seats with controls.CTRL_ELECTRONIC
                    if( controls_at( wreckage, pos ) ) {
                        m.add_spawn( mon_zombie_military_pilot, 1, pos.x, pos.y );
                    } else {
                        if( one_in( 5 ) ) {
                            m.add_spawn( mon_zombie_bio_op, 1, pos.x, pos.y );
                        } else if( one_in( 5 ) ) {
                            m.add_spawn( mon_zombie_scientist, 1, pos.x, pos.y );
                        } else {
                            m.add_spawn( mon_zombie_soldier, 1, pos.x, pos.y );
                        }
                    }

                    // Delete the items that would have spawned here from a "corpse"
                    for( auto sp : wreckage->parts_at_relative( vp.mount(), true ) ) {
                        vehicle_stack here = wreckage->get_items( sp );

                        for( auto iter = here.begin(); iter != here.end(); ) {
                            iter = here.erase( iter );
                        }
                    }
                }
                break;
            case 4:
            case 5: // 2/3rds clown car
                for( const vpart_reference &vp : wreckage->get_any_parts( VPFLAG_SEATBELT ) ) {
                    const tripoint pos = vp.pos();
                    // Spawn pilots in seats with controls.
                    if( controls_at( wreckage, pos ) ) {
                        m.add_spawn( mon_zombie_military_pilot, 1, pos.x, pos.y );
                    } else {
                        if( !one_in( 3 ) ) {
                            m.add_spawn( mon_zombie_soldier, 1, pos.x, pos.y );
                        }
                    }

                    // Delete the items that would have spawned here from a "corpse"
                    for( auto sp : wreckage->parts_at_relative( vp.mount(), true ) ) {
                        vehicle_stack here = wreckage->get_items( sp );

                        for( auto iter = here.begin(); iter != here.end(); ) {
                            iter = here.erase( iter );
                        }
                    }
                }
                break;
            case 6: // Just pilots
                for( const vpart_reference &vp : wreckage->get_any_parts( VPFLAG_CONTROLS ) ) {
                    const tripoint pos = vp.pos();
                    m.add_spawn( mon_zombie_military_pilot, 1, pos.x, pos.y );

                    // Delete the items that would have spawned here from a "corpse"
                    for( auto sp : wreckage->parts_at_relative( vp.mount(), true ) ) {
                        vehicle_stack here = wreckage->get_items( sp );

                        for( auto iter = here.begin(); iter != here.end(); ) {
                            iter = here.erase( iter );
                        }
                    }
                }
                break;
            case 7: // Empty clown car
            case 8:
                break;
            default:
                break;
        }
        if( !one_in( 4 ) ) {
            wreckage->smash( 0.8f, 1.2f, 1.0f, point( dice( 1, 8 ) - 5, dice( 1, 8 ) - 5 ), 6 + dice( 1, 10 ) );
        } else {
            wreckage->smash( 0.1f, 0.9f, 1.0f, point( dice( 1, 8 ) - 5, dice( 1, 8 ) - 5 ), 6 + dice( 1, 10 ) );
        }
    }
}

void mx_military( map &m, const tripoint & )
{
    int num_bodies = dice( 2, 6 );
    for( int i = 0; i < num_bodies; i++ ) {
        if( const auto p = random_point( m, [&m]( const tripoint & n ) {
        return m.passable( n );
        } ) ) {
            if( one_in( 10 ) ) {
                m.add_spawn( mon_zombie_soldier, 1, p->x, p->y );
            } else if( one_in( 25 ) ) {
                if( one_in( 2 ) ) {
                    m.add_spawn( mon_zombie_bio_op, 1, p->x, p->y );
                } else {
                    m.add_spawn( mon_zombie_grenadier, 1, p->x, p->y );
                }
            } else {
                m.place_items( "map_extra_military", 100, *p, *p, true, 0 );
            }
        }

    }
    static const std::array<mtype_id, 4> monsters = { {
            mon_gelatin, mon_mi_go, mon_kreck, mon_gracke,
        }
    };
    int num_monsters = rng( 0, 3 );
    for( int i = 0; i < num_monsters; i++ ) {
        const mtype_id &type = random_entry( monsters );
        int mx = rng( 1, SEEX * 2 - 2 ), my = rng( 1, SEEY * 2 - 2 );
        m.add_spawn( type, 1, mx, my );
    }
    m.place_spawns( GROUP_MAYBE_MIL, 2, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1,
                    0.1f ); //0.1 = 1-5
    m.place_items( "rare", 25, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0 );
}

void mx_science( map &m, const tripoint & )
{
    int num_bodies = dice( 2, 5 );
    for( int i = 0; i < num_bodies; i++ ) {
        if( const auto p = random_point( m, [&m]( const tripoint & n ) {
        return m.passable( n );
        } ) ) {
            if( one_in( 10 ) ) {
                m.add_spawn( mon_zombie_scientist, 1, p->x, p->y );
            } else {
                m.place_items( "map_extra_science", 100, *p, *p, true, 0 );
            }
        }
    }
    static const std::array<mtype_id, 4> monsters = { {
            mon_gelatin, mon_mi_go, mon_kreck, mon_gracke
        }
    };
    int num_monsters = rng( 0, 3 );
    for( int i = 0; i < num_monsters; i++ ) {
        const mtype_id &type = random_entry( monsters );
        int mx = rng( 1, SEEX * 2 - 2 ), my = rng( 1, SEEY * 2 - 2 );
        m.add_spawn( type, 1, mx, my );
    }
    m.place_items( "rare", 45, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0 );
}

void mx_collegekids( map &m, const tripoint & )
{
    //college kids that got into trouble
    int num_bodies = dice( 2, 6 );
    int type = dice( 1, 10 );

    for( int i = 0; i < num_bodies; i++ ) {
        if( const auto p = random_point( m, [&m]( const tripoint & n ) {
        return m.passable( n );
        } ) ) {
            if( one_in( 10 ) ) {
                m.add_spawn( mon_zombie_tough, 1, p->x, p->y );
            } else {
                if( type < 6 ) { // kids going to a cabin in the woods
                    m.place_items( "map_extra_college_camping", 100, *p, *p, true, 0 );
                } else if( type < 9 ) { // kids going to a sporting event
                    m.place_items( "map_extra_college_sports", 100, *p, *p, true, 0 );
                } else { // kids going to a lake
                    m.place_items( "map_extra_college_lake", 100, *p, *p, true, 0 );
                }
            }
        }
    }
    static const std::array<mtype_id, 4> monsters = { {
            mon_gelatin, mon_mi_go, mon_kreck, mon_gracke
        }
    };
    int num_monsters = rng( 0, 3 );
    for( int i = 0; i < num_monsters; i++ ) {
        const mtype_id &type = random_entry( monsters );
        int mx = rng( 1, SEEX * 2 - 2 ), my = rng( 1, SEEY * 2 - 2 );
        m.add_spawn( type, 1, mx, my );
    }
}

void mx_roadblock( map &m, const tripoint &abs_sub )
{
    std::string north = overmap_buffer.ter( abs_sub.x / 2, abs_sub.y / 2 - 1, abs_sub.z ).id().c_str();
    std::string south = overmap_buffer.ter( abs_sub.x / 2, abs_sub.y / 2 + 1, abs_sub.z ).id().c_str();
    std::string west = overmap_buffer.ter( abs_sub.x / 2 - 1, abs_sub.y / 2, abs_sub.z ).id().c_str();
    std::string east = overmap_buffer.ter( abs_sub.x / 2 + 1, abs_sub.y / 2, abs_sub.z ).id().c_str();

    bool northroad = false;
    bool eastroad = false;
    bool southroad = false;
    bool westroad = false;

    if( north.find( "road_" ) == 0 ) {
        northroad = true;
    }
    if( east.find( "road_" ) == 0 ) {
        eastroad = true;
    }
    if( south.find( "road_" ) == 0 ) {
        southroad = true;
    }
    if( west.find( "road_" ) == 0 ) {
        westroad = true;
    }

    const auto spawn_turret = [&]( int x, int y ) {
        if( one_in( 2 ) ) {
            m.add_spawn( mon_turret_bmg, 1, x, y );
        } else {
            m.add_spawn( mon_turret_rifle, 1, x, y );
        }
    };
    bool mil = false;
    if( one_in( 3 ) ) {
        mil = true;
    }
    if( mil ) { //Military doesn't joke around with their barricades!

        if( northroad ) {
            line( &m, t_fence_barbed, 4, 3, 10, 3 );
            line( &m, t_fence_barbed, 13, 3, 19, 3 );
        }
        if( eastroad ) {
            line( &m, t_fence_barbed, SEEX * 2 - 3, 4, SEEX * 2 - 3, 10 );
            line( &m, t_fence_barbed, SEEX * 2 - 3, 13, SEEX * 2 - 3, 19 );
        }
        if( southroad ) {
            line( &m, t_fence_barbed, 4, SEEY * 2 - 3, 10, SEEY * 2 - 3 );
            line( &m, t_fence_barbed, 13, SEEY * 2 - 3, 19, SEEY * 2 - 3 );
        }
        if( eastroad ) {
            line( &m, t_fence_barbed, 3, 4, 3, 10 );
            line( &m, t_fence_barbed, 3, 13, 3, 19 );
        }
        if( one_in( 3 ) ) { // Chicken delivery
            m.add_vehicle( vgroup_id( "military_vehicles" ), tripoint( 12, SEEY * 2 - 7, abs_sub.z ), 0, 70,
                           -1 );
            m.add_spawn( mon_chickenbot, 1, 12, 12 );
        } else if( one_in( 2 ) ) { // TAAANK
            // The truck's wrecked...with fuel.  Explosive barrel?
            m.add_vehicle( vproto_id( "military_cargo_truck" ), 12, SEEY * 2 - 8, 0, 70, -1 );
            m.add_spawn( mon_tankbot, 1, 12, 12 );
        } else {  // Vehicle & turrets
            m.add_vehicle( vgroup_id( "military_vehicles" ), tripoint( 12, SEEY * 2 - 10, abs_sub.z ), 0, 70,
                           -1 );
            if( northroad ) {
                spawn_turret( 12, 6 );
            }
            if( eastroad ) {
                spawn_turret( 18, 12 );
            }
            if( southroad ) {
                spawn_turret( 12, 18 );
            }
            if( westroad ) {
                spawn_turret( 6, 12 );
            }
        }

        int num_bodies = dice( 2, 5 );
        for( int i = 0; i < num_bodies; i++ ) {
            if( const auto p = random_point( m, [&m]( const tripoint & n ) {
            return m.passable( n );
            } ) ) {
                m.place_items( "map_extra_military", 100, *p, *p, true, 0 );

                int splatter_range = rng( 1, 3 );
                for( int j = 0; j <= splatter_range; j++ ) {
                    m.add_field( {p->x - ( j * 1 ), p->y + ( j * 1 ), p->z}, fd_blood, 1, 0_turns );
                }
            }
        }
    } else { // Police roadblock

        if( northroad ) {
            line_furn( &m, f_barricade_road, 4, 3, 10, 3 );
            line_furn( &m, f_barricade_road, 13, 3, 19, 3 );
            m.add_spawn( mon_turret, 1, 12, 1 );
        }
        if( eastroad ) {
            line_furn( &m, f_barricade_road, SEEX * 2 - 3, 4, SEEX * 2 - 3, 10 );
            line_furn( &m, f_barricade_road, SEEX * 2 - 3, 13, SEEX * 2 - 3, 19 );
            m.add_spawn( mon_turret, 1, SEEX * 2 - 1, 12 );
        }
        if( southroad ) {
            line_furn( &m, f_barricade_road, 4, SEEY * 2 - 3, 10, SEEY * 2 - 3 );
            line_furn( &m, f_barricade_road, 13, SEEY * 2 - 3, 19, SEEY * 2 - 3 );
            m.add_spawn( mon_turret, 1, 12, SEEY * 2 - 1 );
        }
        if( westroad ) {
            line_furn( &m, f_barricade_road, 3, 4, 3, 10 );
            line_furn( &m, f_barricade_road, 3, 13, 3, 19 );
            m.add_spawn( mon_turret, 1, 1, 12 );
        }

        m.add_vehicle( vproto_id( "policecar" ), 8, 6, 20 );
        m.add_vehicle( vproto_id( "policecar" ), 16, SEEY * 2 - 6, 145 );
        int num_bodies = dice( 1, 6 );
        for( int i = 0; i < num_bodies; i++ ) {
            if( const auto p = random_point( m, [&m]( const tripoint & n ) {
            return m.passable( n );
            } ) ) {
                m.place_items( "map_extra_police", 100, *p, *p, true, 0 );

                int splatter_range = rng( 1, 3 );
                for( int j = 0; j <= splatter_range; j++ ) {
                    m.add_field( {p->x + ( j * 1 ), p->y - ( j * 1 ), p->z}, fd_blood, 1, 0_turns );
                }
            }
        }
    }
}

void mx_drugdeal( map &m, const tripoint &abs_sub )
{
    // Decide on a drug type
    int num_drugs = 0;
    itype_id drugtype;
    switch( rng( 1, 10 ) ) {
        case 1: // Weed
            num_drugs = rng( 20, 30 );
            drugtype = "weed";
            break;
        case 2:
        case 3:
        case 4:
        case 5: // Cocaine
            num_drugs = rng( 10, 20 );
            drugtype = "coke";
            break;
        case 6:
        case 7:
        case 8: // Meth
            num_drugs = rng( 8, 14 );
            drugtype = "meth";
            break;
        case 9:
        case 10: // Heroin
            num_drugs = rng( 6, 12 );
            drugtype = "heroin";
            break;
    }
    int num_bodies_a = dice( 3, 3 );
    int num_bodies_b = dice( 3, 3 );
    bool north_south = one_in( 2 );
    bool a_has_drugs = one_in( 2 );

    for( int i = 0; i < num_bodies_a; i++ ) {
        int x = 0;
        int y = 0;
        int x_offset = 0;
        int y_offset = 0;
        int tries = 0;
        do { // Loop until we find a valid spot to dump a body, or we give up
            if( north_south ) {
                x = rng( 0, SEEX * 2 - 1 );
                y = rng( 0, SEEY - 4 );
                x_offset = 0;
                y_offset = -1;
            } else {
                x = rng( 0, SEEX - 4 );
                y = rng( 0, SEEY * 2 - 1 );
                x_offset = -1;
                y_offset = 0;
            }
            tries++;
        } while( tries < 10 && m.impassable( x, y ) );

        if( tries < 10 ) { // We found a valid spot!
            if( one_in( 10 ) ) {
                m.add_spawn( mon_zombie_spitter, 1, x, y );
            } else {
                m.place_items( "map_extra_drugdeal", 100, x, y, x, y, true, 0 );
                int splatter_range = rng( 1, 3 );
                for( int j = 0; j <= splatter_range; j++ ) {
                    m.add_field( {x + ( j * x_offset ), y + ( j * y_offset ), abs_sub.z},
                                 fd_blood, 1, 0_turns );
                }
            }
            if( a_has_drugs && num_drugs > 0 ) {
                int drugs_placed = rng( 2, 6 );
                if( drugs_placed > num_drugs ) {
                    drugs_placed = num_drugs;
                    num_drugs = 0;
                }
                m.spawn_item( x, y, drugtype, 0, drugs_placed );
            }
        }
    }
    for( int i = 0; i < num_bodies_b; i++ ) {
        int x = 0;
        int y = 0;
        int x_offset = 0;
        int y_offset = 0;
        int tries = 0;
        do { // Loop until we find a valid spot to dump a body, or we give up
            if( north_south ) {
                x = rng( 0, SEEX * 2 - 1 );
                y = rng( SEEY + 3, SEEY * 2 - 1 );
                x_offset = 0;
                y_offset = 1;
            } else {
                x = rng( SEEX + 3, SEEX * 2 - 1 );
                y = rng( 0, SEEY * 2 - 1 );
                x_offset = 1;
                y_offset = 0;
            }
            tries++;
        } while( tries < 10 && m.impassable( x, y ) );

        if( tries < 10 ) { // We found a valid spot!
            if( one_in( 20 ) ) {
                m.add_spawn( mon_zombie_smoker, 1, x, y );
            } else {
                m.place_items( "map_extra_drugdeal", 100, x, y, x, y, true, 0 );
                int splatter_range = rng( 1, 3 );
                for( int j = 0; j <= splatter_range; j++ ) {
                    m.add_field( {x + ( j * x_offset ), y + ( j * y_offset ), abs_sub.z},
                                 fd_blood, 1, 0_turns );
                }
                if( !a_has_drugs && num_drugs > 0 ) {
                    int drugs_placed = rng( 2, 6 );
                    if( drugs_placed > num_drugs ) {
                        drugs_placed = num_drugs;
                        num_drugs = 0;
                    }
                    m.spawn_item( x, y, drugtype, 0, drugs_placed );
                }
            }
        }
    }
    static const std::array<mtype_id, 4> monsters = { {
            mon_gelatin, mon_mi_go, mon_kreck, mon_gracke
        }
    };
    int num_monsters = rng( 0, 3 );
    for( int i = 0; i < num_monsters; i++ ) {
        const mtype_id &type = random_entry( monsters );
        int mx = rng( 1, SEEX * 2 - 2 ), my = rng( 1, SEEY * 2 - 2 );
        m.add_spawn( type, 1, mx, my );
    }
}

void mx_supplydrop( map &m, const tripoint &/*abs_sub*/ )
{
    int num_crates = rng( 1, 5 );
    for( int i = 0; i < num_crates; i++ ) {
        const auto p = random_point( m, [&m]( const tripoint & n ) {
            return m.passable( n );
        } );
        if( !p ) {
            break;
        }
        m.furn_set( p->x, p->y, f_crate_c );
        std::string item_group;
        switch( rng( 1, 10 ) ) {
            case 1:
            case 2:
            case 3:
            case 4:
                item_group = "mil_food";
                break;
            case 5:
            case 6:
            case 7:
                item_group = "grenades";
                break;
            case 8:
            case 9:
                item_group = "mil_armor";
                break;
            case 10:
                item_group = "guns_rifle_milspec";
                break;
        }
        int items_created = 0;
        for( int i = 0; i < 10 && items_created < 2; i++ ) {
            items_created += m.place_items( item_group, 80, *p, *p, true, 0, 100 ).size();
        }
        if( m.i_at( *p ).empty() ) {
            m.destroy( *p, true );
        }
    }
}

void mx_portal( map &m, const tripoint &abs_sub )
{
    static const std::array<mtype_id, 5> monsters = { {
            mon_gelatin, mon_flaming_eye, mon_kreck, mon_gracke, mon_blank
        }
    };
    int x = rng( 1, SEEX * 2 - 2 ), y = rng( 1, SEEY * 2 - 2 );
    for( int i = x - 1; i <= x + 1; i++ ) {
        for( int j = y - 1; j <= y + 1; j++ ) {
            m.make_rubble( tripoint( i,  j, abs_sub.z ), f_rubble_rock, true );
        }
    }
    mtrap_set( &m, x, y, tr_portal );
    int num_monsters = rng( 0, 4 );
    for( int i = 0; i < num_monsters; i++ ) {
        const mtype_id &type = random_entry( monsters );
        int mx = rng( 1, SEEX * 2 - 2 ), my = rng( 1, SEEY * 2 - 2 );
        m.make_rubble( tripoint( mx,  my, abs_sub.z ), f_rubble_rock, true );
        m.add_spawn( type, 1, mx, my );
    }
}

void mx_minefield( map &m, const tripoint &abs_sub )
{
    int num_mines = rng( 6, 20 );
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            if( one_in( 3 ) ) {
                m.ter_set( x, y, t_dirt );
            }
        }
    }
    for( int i = 0; i < num_mines; i++ ) {
        // No mines at the extreme edges: safe to walk on a sign tile
        int x = rng( 1, SEEX * 2 - 2 ), y = rng( 1, SEEY * 2 - 2 );
        if( !m.has_flag( "DIGGABLE", x, y ) || one_in( 8 ) ) {
            m.ter_set( x, y, t_dirtmound );
        }
        mtrap_set( &m, x, y, tr_landmine_buried );
    }
    int x1 = 0;
    int y1 = 0;
    int x2 = ( SEEX * 2 - 1 );
    int y2 = ( SEEY * 2 - 1 );
    m.furn_set( x1, y1, furn_str_id( "f_sign" ) );
    m.set_signage( tripoint( x1,  y1, abs_sub.z ), _( "DANGER! MINEFIELD!" ) );
    m.furn_set( x1, y2, furn_str_id( "f_sign" ) );
    m.set_signage( tripoint( x1,  y2, abs_sub.z ), _( "DANGER! MINEFIELD!" ) );
    m.furn_set( x2, y1, furn_str_id( "f_sign" ) );
    m.set_signage( tripoint( x2,  y1, abs_sub.z ), _( "DANGER! MINEFIELD!" ) );
    m.furn_set( x2, y2, furn_str_id( "f_sign" ) );
    m.set_signage( tripoint( x2,  y2, abs_sub.z ), _( "DANGER! MINEFIELD!" ) );
}

void mx_crater( map &m, const tripoint &abs_sub )
{
    int size = rng( 2, 6 );
    int size_squared = size * size;
    int x = rng( size, SEEX * 2 - 1 - size ), y = rng( size, SEEY * 2 - 1 - size );
    for( int i = x - size; i <= x + size; i++ ) {
        for( int j = y - size; j <= y + size; j++ ) {
            //If we're using circular distances, make circular craters
            //Pythagoras to the rescue, x^2 + y^2 = hypotenuse^2
            if( !trigdist || ( ( ( i - x ) * ( i - x ) + ( j - y ) * ( j - y ) ) <= size_squared ) ) {
                m.destroy( tripoint( i,  j, abs_sub.z ), true );
                m.adjust_radiation( i, j, rng( 20, 40 ) );
            }
        }
    }
}

void mx_fumarole( map &m, const tripoint & )
{
    int x1 = rng( 0,    SEEX     - 1 ), y1 = rng( 0,    SEEY     - 1 ),
        x2 = rng( SEEX, SEEX * 2 - 1 ), y2 = rng( SEEY, SEEY * 2 - 1 );
    std::vector<point> fumarole = line_to( x1, y1, x2, y2, 0 );
    for( auto &i : fumarole ) {
        m.ter_set( i.x, i.y, t_lava );
        if( one_in( 6 ) ) {
            m.spawn_item( i.x - 1, i.y - 1, "chunk_sulfur" );
        }
    }
}

void mx_portal_in( map &m, const tripoint &abs_sub )
{
    static const std::array<mtype_id, 5> monsters = { {
            mon_gelatin, mon_flaming_eye, mon_kreck, mon_gracke, mon_blank
        }
    };
    int x = rng( 5, SEEX * 2 - 6 ), y = rng( 5, SEEY * 2 - 6 );
    m.add_field( {x, y, abs_sub.z}, fd_fatigue, 3, 0_turns );
    fungal_effects fe( *g, m );
    for( int i = x - 5; i <= x + 5; i++ ) {
        for( int j = y - 5; j <= y + 5; j++ ) {
            if( rng( 1, 9 ) >= trig_dist( x, y, i, j ) ) {
                fe.marlossify( tripoint( i, j, abs_sub.z ) );
                if( one_in( 15 ) ) {
                    m.add_spawn( random_entry( monsters ), 1, i, j );
                }
            }
        }
    }
}

void mx_anomaly( map &m, const tripoint &abs_sub )
{
    tripoint center( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), abs_sub.z );
    artifact_natural_property prop =
        artifact_natural_property( rng( ARTPROP_NULL + 1, ARTPROP_MAX - 1 ) );
    m.create_anomaly( center, prop );
    m.spawn_natural_artifact( center, prop );
}

void mx_shia( map &m, const tripoint & )
{
    // A rare chance to spawn Shia. This was extracted from the hardcoded forest mapgen
    // and moved into a map extra, but it still has a one_in chance of spawning because
    // otherwise the extreme rarity of this event wildly skewed the values for all of the
    // other extras.
    if( one_in( 5000 ) ) {
        m.add_spawn( mon_shia, 1, SEEX, SEEY );
    }
}

void mx_spider( map &m, const tripoint &abs_sub )
{
    // This was extracted from the hardcoded forest mapgen and slightly altered so
    // that it used flags rather than specific terrain types in determining where to
    // place webs.
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint location( i, j, abs_sub.z );

            bool should_web_flat = m.has_flag_ter( "FLAT", location ) && !one_in( 3 );
            bool should_web_shrub = m.has_flag_ter( "SHRUB", location ) && !one_in( 4 );
            bool should_web_tree = m.has_flag_ter( "TREE", location ) && !one_in( 4 );

            if( should_web_flat || should_web_shrub || should_web_tree ) {
                m.add_field( location, fd_web, rng( 1, 3 ), 0_turns );
            }
        }
    }

    m.ter_set( 12, 12, t_dirt );
    m.furn_set( 12, 12, f_egg_sackws );
    m.remove_field( { 12, 12, m.get_abs_sub().z }, fd_web );
    m.add_spawn( mon_spider_web, rng( 1, 2 ), SEEX, SEEY );
}

void mx_jabberwock( map &m, const tripoint & )
{
    // A rare chance to spawn a jabberwock. This was extracted from the harcoded forest mapgen
    // and moved into a map extra. It still has a one_in chance of spawning because otherwise
    // the rarity skewed the values for all the other extras too much. I considered moving it
    // into the monster group, but again the hardcoded rarity it had in the forest mapgen was
    // not easily replicated there.
    if( one_in( 50 ) ) {
        m.add_spawn( mon_jabberwock, 1, SEEX, SEEY );
    }
}

void mx_grove( map &m, const tripoint &abs_sub )
{
    // From wikipedia - The main meaning of "grove" is a group of trees that grow close together,
    // generally without many bushes or other plants underneath.

    // This map extra finds the first tree in the area, and then converts all trees, young trees,
    // and shrubs in the area into that type of tree.

    ter_id tree;
    bool found_tree = false;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint location( i, j, abs_sub.z );
            if( m.has_flag_ter( "TREE", location ) ) {
                tree = m.ter( location );
                found_tree = true;
            }
        }
    }

    if( !found_tree ) {
        return;
    }

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint location( i, j, abs_sub.z );
            if( m.has_flag_ter( "SHRUB", location ) || m.has_flag_ter( "TREE", location ) ||
                m.has_flag_ter( "YOUNG", location ) ) {
                m.ter_set( location, tree );
            }
        }
    }
}

void mx_shrubbery( map &m, const tripoint &abs_sub )
{
    // This map extra finds the first shrub in the area, and then converts all trees, young trees,
    // and shrubs in the area into that type of shrub.

    ter_id shrubbery;
    bool found_shrubbery = false;
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint location( i, j, abs_sub.z );
            if( m.has_flag_ter( "SHRUB", location ) ) {
                shrubbery = m.ter( location );
                found_shrubbery = true;
            }
        }
    }

    if( !found_shrubbery ) {
        return;
    }

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint location( i, j, abs_sub.z );
            if( m.has_flag_ter( "SHRUB", location ) || m.has_flag_ter( "TREE", location ) ||
                m.has_flag_ter( "YOUNG", location ) ) {
                m.ter_set( location, shrubbery );
            }
        }
    }
}

void mx_clearcut( map &m, const tripoint &abs_sub )
{
    // From wikipedia - Clearcutting, clearfelling or clearcut logging is a forestry/logging
    // practice in which most or all trees in an area are uniformly cut down.

    // This map extra converts all trees and young trees in the area to stumps.

    ter_id stump( "t_stump" );

    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int j = 0; j < SEEY * 2; j++ ) {
            const tripoint location( i, j, abs_sub.z );
            if( m.has_flag_ter( "TREE", location ) || m.has_flag_ter( "YOUNG", location ) ) {
                m.ter_set( location, stump );
            }
        }
    }
}

void mx_pond( map &m, const tripoint &abs_sub )
{
    // This map extra creates small ponds using a simple cellular automaton.

    constexpr int width = SEEX * 2;
    constexpr int height = SEEY * 2;

    // Generate the cells for our lake.
    std::vector<std::vector<int>> current = CellularAutomata::generate_cellular_automaton( width,
                                            height, 55, 5, 4, 3 );

    // Loop through and turn every live cell into water.
    // Do a roll for our three possible lake types:
    // - all deep water
    // - all shallow water
    // - shallow water on the shore, deep water in the middle
    const int lake_type = rng( 1, 3 );
    for( int i = 0; i < width; i++ ) {
        for( int j = 0; j < height; j++ ) {
            if( current[i][j] == 1 ) {
                const tripoint location( i, j, abs_sub.z );
                m.furn_set( location, f_null );

                switch( lake_type ) {
                    case 1:
                        m.ter_set( location, t_water_sh );
                        break;
                    case 2:
                        m.ter_set( location, t_water_dp );
                        break;
                    case 3:
                        const int neighbors = CellularAutomata::neighbor_count( current, width, height, i, j );
                        if( neighbors == 8 ) {
                            m.ter_set( location, t_water_dp );
                        } else {
                            m.ter_set( location, t_water_sh );
                        }
                        break;
                }
            }
        }
    }

    m.place_spawns( GROUP_FISH, 1, 0, 0, width, height, 0.15f );
}

void mx_clay_deposit( map &m, const tripoint &abs_sub )
{
    // This map extra creates small clay deposits using a simple cellular automaton.

    constexpr int width = SEEX * 2;
    constexpr int height = SEEY * 2;

    for( int tries = 0; tries < 5; tries++ ) {
        // Generate the cells for our clay deposit.
        std::vector<std::vector<int>> current = CellularAutomata::generate_cellular_automaton( width,
                                                height, 35, 5, 4, 3 );

        // With our settings for the CA, it's sometimes possible to get a bad generation with not enough
        // alive cells (or even 0).
        int alive_count = 0;
        for( int i = 0; i < width; i++ ) {
            for( int j = 0; j < height; j++ ) {
                alive_count += current[i][j];
            }
        }

        // If we have fewer than 4 alive cells, lets try again.
        if( alive_count < 4 ) {
            continue;
        }

        // Loop through and turn every live cell into clay.
        for( int i = 0; i < width; i++ ) {
            for( int j = 0; j < height; j++ ) {
                if( current[i][j] == 1 ) {
                    const tripoint location( i, j, abs_sub.z );
                    m.furn_set( location, f_null );
                    m.ter_set( location, t_clay );
                }
            }
        }

        // If we got here, it meant we had a successful try and can just break out of
        // our retry loop.
        break;
    }
}

typedef std::unordered_map<std::string, map_special_pointer> FunctionMap;
FunctionMap builtin_functions = {
    { "mx_null", mx_null },
    { "mx_helicopter", mx_helicopter },
    { "mx_military", mx_military },
    { "mx_science", mx_science },
    { "mx_collegekids", mx_collegekids },
    { "mx_roadblock", mx_roadblock },
    { "mx_drugdeal", mx_drugdeal },
    { "mx_supplydrop", mx_supplydrop },
    { "mx_portal", mx_portal },
    { "mx_minefield", mx_minefield },
    { "mx_crater", mx_crater },
    { "mx_fumarole", mx_fumarole },
    { "mx_portal_in", mx_portal_in },
    { "mx_anomaly", mx_anomaly },
    { "mx_shia", mx_shia },
    { "mx_spider", mx_spider },
    { "mx_jabberwock", mx_jabberwock },
    { "mx_grove", mx_grove },
    { "mx_shrubbery", mx_shrubbery },
    { "mx_clearcut", mx_clearcut },
    { "mx_pond", mx_pond },
    { "mx_clay_deposit", mx_clay_deposit },
};

map_special_pointer get_function( const std::string &name )
{
    const auto iter = builtin_functions.find( name );
    if( iter == builtin_functions.end() ) {
        debugmsg( "no map special with name %s", name.c_str() );
        return nullptr;
    }
    return iter->second;
}

}
