#include "map_extras.h"

#include "debug.h"
#include "field.h"
#include "fungal_effects.h"
#include "game.h"
#include "item_group.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mapgen_functions.h"
#include "mongroup.h"
#include "mtype.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "rng.h"
#include "trap.h"
#include "vehicle.h"
#include "vehicle_group.h"

namespace MapExtras
{

static const mongroup_id GROUP_MAYBE_MIL( "GROUP_MAYBE_MIL" );

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
static const mtype_id mon_zombie_bio_op( "mon_zombie_bio_op" );
static const mtype_id mon_zombie_grenadier( "mon_zombie_grenadier" );

void mx_null( map &, const tripoint & )
{
    debugmsg( "Tried to generate null map extra." );
}

void mx_helicopter( map &m, const tripoint &abs_sub )
{
    int cx = rng( 4, SEEX * 2 - 5 ), cy = rng( 4, SEEY * 2 - 5 );
    for( int x = 0; x < SEEX * 2; x++ ) {
        for( int y = 0; y < SEEY * 2; y++ ) {
            if( x >= cx - 4 && x <= cx + 4 && y >= cy - 4 && y <= cy + 4 ) {
                if( !one_in( 5 ) ) {
                    m.make_rubble( tripoint( x,  y, abs_sub.z ), f_wreckage, true );
                } else if( m.is_bashable( x, y ) ) {
                    m.destroy( tripoint( x,  y, abs_sub.z ), true );
                }
            } else if( one_in( 10 ) ) { // 1 in 10 chance of being wreckage anyway
                m.make_rubble( tripoint( x,  y, abs_sub.z ), f_wreckage, true );
            }
        }
    }

    m.spawn_item( rng( 5, 18 ), rng( 5, 18 ), "black_box" );
    m.place_items( "helicopter", 90, cx - 4, cy - 4, cx + 4, cy + 4, true, 0 );
    m.place_items( "helicopter", 20, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0 );
    items_location extra_items = "helicopter";
    switch( rng( 1, 4 ) ) {
        case 1:
            extra_items = "military";
            break;
        case 2:
            extra_items = "science";
            break;
        case 3:
            extra_items = "guns_milspec";
            break;
        case 4:
            extra_items = "bionics";
            break;
    }
    m.place_spawns( GROUP_MAYBE_MIL, 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, 0.1f ); //0.1 = 1-5
    m.place_items( extra_items, 70, cx - 4, cy - 4, cx + 4, cy + 4, true, 0, 100, 20 );
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
    // Currently doesn't handle adjacency to turns or intersections well, we may want to abort in future
    bool rotated = false;
    std::string north = overmap_buffer.ter( abs_sub.x / 2, abs_sub.y / 2 - 1, abs_sub.z ).id().c_str();
    std::string south = overmap_buffer.ter( abs_sub.x / 2, abs_sub.y / 2 + 1, abs_sub.z ).id().c_str();
    if( north.find( "road_" ) == 0 && south.find( "road_" ) == 0 ) {
        rotated = true;
        //Rotate the terrain -90 so that all of the items will be in the correct position
        //when the entire map is rotated at the end
        m.rotate( 3 );
    }
    bool mil = false;
    if( one_in( 3 ) ) {
        mil = true;
    }
    if( mil ) { //Military doesn't joke around with their barricades!
        line( &m, t_fence_barbed, SEEX * 2 - 1, 4, SEEX * 2 - 1, 10 );
        line( &m, t_fence_barbed, SEEX * 2 - 3, 13, SEEX * 2 - 3, 19 );
        line( &m, t_fence_barbed, 3, 4, 3, 10 );
        line( &m, t_fence_barbed, 1, 13, 1, 19 );
        if( one_in( 3 ) ) { // Chicken delivery
            m.add_vehicle( vgroup_id( "military_vehicles" ), tripoint( 12, SEEY * 2 - 5, abs_sub.z ), 0, 70,
                           -1 );
            m.add_spawn( mon_chickenbot, 1, 12, 12 );
        } else if( one_in( 2 ) ) { // TAAANK
            // The truck's wrecked...with fuel.  Explosive barrel?
            m.add_vehicle( vproto_id( "military_cargo_truck" ), 12, SEEY * 2 - 5, 0, 70, -1 );
            m.add_spawn( mon_tankbot, 1, 12, 12 );
        } else {  // Vehicle & turrets
            m.add_vehicle( vgroup_id( "military_vehicles" ), tripoint( 12, SEEY * 2 - 5, abs_sub.z ), 0, 70,
                           -1 );
            m.add_spawn( mon_turret_bmg, 1, 12, 12 );
            m.add_spawn( mon_turret_rifle, 1, 9, 12 );
        }

        int num_bodies = dice( 2, 5 );
        for( int i = 0; i < num_bodies; i++ ) {
            if( const auto p = random_point( m, [&m]( const tripoint & n ) {
            return m.passable( n );
            } ) ) {
                m.place_items( "map_extra_military", 100, *p, *p, true, 0 );

                int splatter_range = rng( 1, 3 );
                for( int j = 0; j <= splatter_range; j++ ) {
                    m.add_field( {p->x - ( j * 1 ), p->y + ( j * 1 ), p->z}, fd_blood, 1, 0 );
                }
            }
        }
    } else { // Police roadblock
        line_furn( &m, f_barricade_road, SEEX * 2 - 1, 4, SEEX * 2 - 1, 10 );
        line_furn( &m, f_barricade_road, SEEX * 2 - 3, 13, SEEX * 2 - 3, 19 );
        line_furn( &m, f_barricade_road, 3, 4, 3, 10 );
        line_furn( &m, f_barricade_road, 1, 13, 1, 19 );
        m.add_vehicle( vproto_id( "policecar" ), 8, 5, 20 );
        m.add_vehicle( vproto_id( "policecar" ), 16, SEEY * 2 - 5, 145 );
        m.add_spawn( mon_turret, 1, 1, 12 );
        m.add_spawn( mon_turret, 1, SEEX * 2 - 1, 12 );

        int num_bodies = dice( 1, 6 );
        for( int i = 0; i < num_bodies; i++ ) {
            if( const auto p = random_point( m, [&m]( const tripoint & n ) {
            return m.passable( n );
            } ) ) {
                m.place_items( "map_extra_police", 100, *p, *p, true, 0 );

                int splatter_range = rng( 1, 3 );
                for( int j = 0; j <= splatter_range; j++ ) {
                    m.add_field( {p->x + ( j * 1 ), p->y - ( j * 1 ), p->z}, fd_blood, 1, 0 );
                }
            }
        }
    }
    if( rotated ) {
        m.rotate( 1 );
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
                                 fd_blood, 1, 0 );
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
                                 fd_blood, 1, 0 );
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
    m.add_field( {x, y, abs_sub.z}, fd_fatigue, 3, 0 );
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
    { "mx_anomaly", mx_anomaly }
};

map_special_pointer get_function( const std::string &name )
{
    const auto iter = builtin_functions.find( name );
    if( iter == builtin_functions.end() ) {
        debugmsg( "no map special with name %s", name.c_str() );
        return NULL;
    }
    return iter->second;
}

};
