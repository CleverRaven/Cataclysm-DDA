#include "cata_catch.h"
#include "coordinates.h"
#include "coords_fwd.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "type_id.h"

static void place_obstacle( map &m, const std::vector<tripoint_bub_ms> &places )
{
    const ter_id t_wall_metal( "t_wall_metal" );
    for( const tripoint_bub_ms &p : places ) {
        m.ter_set( p, t_wall_metal );
    }
    m.set_transparency_cache_dirty( 0 );
    m.build_map_cache( 0 );
    for( const tripoint_bub_ms &p : places ) {
        REQUIRE( !m.is_transparent( p ) );
    }
}

static void expect_path( const std::vector<tripoint_bub_ms> &expected,
                         const std::vector<tripoint_bub_ms> &actual )
{
    THEN( "path avoids obstacle" ) {
        CHECK( expected == actual );
    }
}

static map &setup_map_without_obstacles()
{
    const ter_id t_floor( "t_floor" );
    map &here = get_map();
    clear_map();
    const tripoint_bub_ms topleft = tripoint_bub_ms::zero;
    const tripoint_bub_ms bottomright { 20, 20, 0 };
    for( const tripoint_bub_ms &p : here.points_in_rectangle( topleft, bottomright ) ) {
        here.ter_set( p, t_floor );
    }
    return here;
}

TEST_CASE( "find_clear_path_with_adjacent_obstacles", "[map]" )
{
    const tripoint_bub_ms p_center{ 10, 10, 0 };
    // Adjacent mapsquares
    const tripoint_bub_ms nw = p_center + tripoint_rel_ms::north_west;
    const tripoint_bub_ms n = p_center + tripoint_rel_ms::north;
    const tripoint_bub_ms ne = p_center + tripoint_rel_ms::north_east;
    const tripoint_bub_ms e = p_center + tripoint_rel_ms::east;
    const tripoint_bub_ms se = p_center + tripoint_rel_ms::south_east;
    const tripoint_bub_ms s = p_center + tripoint_rel_ms::south;
    const tripoint_bub_ms sw = p_center + tripoint_rel_ms::south_west;
    const tripoint_bub_ms w = p_center + tripoint_rel_ms::west;
    // Mapsquares further out
    const tripoint_bub_ms n_nw = n + tripoint_rel_ms::north_west;
    const tripoint_bub_ms n_n = n + tripoint_rel_ms::north;
    const tripoint_bub_ms n_ne = n + tripoint_rel_ms::north_east;
    const tripoint_bub_ms e_ne = e + tripoint_rel_ms::north_east;
    const tripoint_bub_ms e_e = e + tripoint_rel_ms::east;
    const tripoint_bub_ms e_se = e + tripoint_rel_ms::south_east;
    const tripoint_bub_ms s_se = s + tripoint_rel_ms::south_east;
    const tripoint_bub_ms s_s = s + tripoint_rel_ms::south;
    const tripoint_bub_ms s_sw = s + tripoint_rel_ms::south_west;
    const tripoint_bub_ms w_sw = w + tripoint_rel_ms::south_west;
    const tripoint_bub_ms w_w = w + tripoint_rel_ms::west;
    const tripoint_bub_ms w_nw = w + tripoint_rel_ms::north_west;

    map &here = setup_map_without_obstacles();
    GIVEN( "Map has adjacent obstacle directly nw, ne, se, sw" ) {
        place_obstacle( here, { nw, ne, se, sw } );
        WHEN( "map::find_clear_path to n_nw" ) {
            expect_path( { n, n_nw }, here.find_clear_path( p_center, n_nw ) );
        }
        WHEN( "map::find_clear_path to n_n" ) {
            expect_path( { n, n_n }, here.find_clear_path( p_center, n_n ) );
        }
        WHEN( "map::find_clear_path to n_ne" ) {
            expect_path( { n, n_ne }, here.find_clear_path( p_center, n_ne ) );
        }
        WHEN( "map::find_clear_path to e_ne" ) {
            expect_path( { e, e_ne }, here.find_clear_path( p_center, e_ne ) );
        }
        WHEN( "map::find_clear_path to e_e" ) {
            expect_path( { e, e_e }, here.find_clear_path( p_center, e_e ) );
        }
        WHEN( "map::find_clear_path to e_se" ) {
            expect_path( { e, e_se }, here.find_clear_path( p_center, e_se ) );
        }
        WHEN( "map::find_clear_path to s_se" ) {
            expect_path( { s, s_se }, here.find_clear_path( p_center, s_se ) );
        }
        WHEN( "map::find_clear_path to s_s" ) {
            expect_path( { s, s_s }, here.find_clear_path( p_center, s_s ) );
        }
        WHEN( "map::find_clear_path to s_sw" ) {
            expect_path( { s, s_sw }, here.find_clear_path( p_center, s_sw ) );
        }
        WHEN( "map::find_clear_path to w_sw" ) {
            expect_path( { w, w_sw }, here.find_clear_path( p_center, w_sw ) );
        }
        WHEN( "map::find_clear_path to w_w" ) {
            expect_path( { w, w_w }, here.find_clear_path( p_center, w_w ) );
        }
        WHEN( "map::find_clear_path to w_nw" ) {
            expect_path( { w, w_nw }, here.find_clear_path( p_center, w_nw ) );
        }
    }
    GIVEN( "Map has adjacent obstacle directly n, e, s, w" ) {
        place_obstacle( here, { n, e, s, w } );
        WHEN( "map::find_clear_path to n_nw" ) {
            expect_path( { nw, n_nw }, here.find_clear_path( p_center, n_nw ) );
        }
        WHEN( "map::find_clear_path to n_ne" ) {
            expect_path( { ne, n_ne }, here.find_clear_path( p_center, n_ne ) );
        }
        WHEN( "map::find_clear_path to e_ne" ) {
            expect_path( { ne, e_ne }, here.find_clear_path( p_center, e_ne ) );
        }
        WHEN( "map::find_clear_path to e_se" ) {
            expect_path( { se, e_se }, here.find_clear_path( p_center, e_se ) );
        }
        WHEN( "map::find_clear_path to s_se" ) {
            expect_path( { se, s_se }, here.find_clear_path( p_center, s_se ) );
        }
        WHEN( "map::find_clear_path to s_sw" ) {
            expect_path( { sw, s_sw }, here.find_clear_path( p_center, s_sw ) );
        }
        WHEN( "map::find_clear_path to w_sw" ) {
            expect_path( { sw, w_sw }, here.find_clear_path( p_center, w_sw ) );
        }
        WHEN( "map::find_clear_path to w_nw" ) {
            expect_path( { nw, w_nw }, here.find_clear_path( p_center, w_nw ) );
        }
    }
    clear_map();
}


TEST_CASE( "find_clear_path_5tiles_with_obstacles", "[map]" )
{
    map &here = setup_map_without_obstacles();
    const tripoint_bub_ms source{ 10, 10, 0 };
    const tripoint_bub_ms target{ 5, 8, 0 };
    GIVEN( "Map has obstacle southeast of target" ) {
        /*
         * Map layout:   t=target  s=source  o=obstacle
         * t . . . . . .
         * . o . . . . .
         * . . . . . s .
         */
        const tripoint_bub_ms obstacle{ 6, 9, 0 };
        place_obstacle( here, { obstacle } );
        WHEN( "map::find_clear_path to target" ) {
            const std::vector<tripoint_bub_ms> path = here.find_clear_path( source, target );
            THEN( "path avoids obstacle" ) {
                CHECK( path[0] == tripoint_bub_ms{ 9, 9, 0 } );
                CHECK( path[1] == tripoint_bub_ms{ 8, 9, 0 } );
                CHECK( path[2] == tripoint_bub_ms{ 7, 9, 0 } );
                CHECK( path[3] == tripoint_bub_ms{ 6, 8, 0 } );
                CHECK( path[4] == tripoint_bub_ms{ 5, 8, 0 } );
                const bool path_contains_obstacle = std::find( path.begin(), path.end(), obstacle ) != path.end();
                CHECK_FALSE( path_contains_obstacle );
            }
            CHECK( here.sees( source, target, -1 ) );
            CHECK( here.sees( target, source, -1 ) );
        }
    }
    GIVEN( "Map has obstacle east of target" ) {
        /*
         * Map layout:   t=target  s=source  o=obstacle
         * t o . . . . .
         * . . . . . . .
         * . . . . . s .
         */
        const tripoint_bub_ms obstacle{ 6, 8, 0 };
        place_obstacle( here, { obstacle } );
        WHEN( "map::find_clear_path to target" ) {
            const std::vector<tripoint_bub_ms> path = here.find_clear_path( source, target );
            THEN( "path avoids obstacle" ) {
                CHECK( path[0] == tripoint_bub_ms{ 9, 10, 0 } );
                CHECK( path[1] == tripoint_bub_ms{ 8, 9, 0 } );
                CHECK( path[2] == tripoint_bub_ms{ 7, 9, 0 } );
                CHECK( path[3] == tripoint_bub_ms{ 6, 9, 0 } );
                CHECK( path[4] == tripoint_bub_ms{ 5, 8, 0 } );
                const bool path_contains_obstacle = std::find( path.begin(), path.end(), obstacle ) != path.end();
                CHECK_FALSE( path_contains_obstacle );
            }
            CHECK( here.sees( source, target, -1 ) );
            CHECK( here.sees( target, source, -1 ) );
        }
    }
    GIVEN( "Map has obstacle northwest of source" ) {
        /*
         * Map layout:   t=target  s=source  o=obstacle
         * t . . . . . .
         * . . . . o . .
         * . . . . . s .
         */
        const tripoint_bub_ms obstacle{ 9, 9, 0 };
        place_obstacle( here, { obstacle } );
        WHEN( "map::find_clear_path to target" ) {
            const std::vector<tripoint_bub_ms> path = here.find_clear_path( source, target );
            THEN( "path avoids obstacle" ) {
                CHECK( path[0] == tripoint_bub_ms{ 9, 10, 0 } );
                CHECK( path[1] == tripoint_bub_ms{ 8, 9, 0 } );
                CHECK( path[2] == tripoint_bub_ms{ 7, 9, 0 } );
                CHECK( path[3] == tripoint_bub_ms{ 6, 8, 0 } );
                CHECK( path[4] == tripoint_bub_ms{ 5, 8, 0 } );
                const bool path_contains_obstacle = std::find( path.begin(), path.end(), obstacle ) != path.end();
                CHECK_FALSE( path_contains_obstacle );
            }
            CHECK( here.sees( source, target, -1 ) );
            CHECK( here.sees( target, source, -1 ) );
        }
    }
    GIVEN( "Map has obstacle west of source" ) {
        /*
         * Map layout:   t=target  s=source  o=obstacle
         * t . . . . . .
         * . . . . . . .
         * . . . . o s .
         */
        const tripoint_bub_ms obstacle{ 9, 10, 0 };
        place_obstacle( here, { obstacle } );
        WHEN( "map::find_clear_path to target" ) {
            const std::vector<tripoint_bub_ms> path = here.find_clear_path( source, target );
            THEN( "path avoids obstacle" ) {
                CHECK( path[0] == tripoint_bub_ms{ 9, 9, 0 } );
                CHECK( path[1] == tripoint_bub_ms{ 8, 9, 0 } );
                CHECK( path[2] == tripoint_bub_ms{ 7, 9, 0 } );
                CHECK( path[3] == tripoint_bub_ms{ 6, 8, 0 } );
                CHECK( path[4] == tripoint_bub_ms{ 5, 8, 0 } );
                const bool path_contains_obstacle = std::find( path.begin(), path.end(), obstacle ) != path.end();
                CHECK_FALSE( path_contains_obstacle );
            }
            CHECK( here.sees( source, target, -1 ) );
            CHECK( here.sees( target, source, -1 ) );
        }
    }
    clear_map();
}

