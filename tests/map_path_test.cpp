#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "field_type.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "monster.h"
#include "pathfinding.h"
#include "point.h"
#include "type_id.h"

static void clear_map_caches( map &m )
{
    m.set_transparency_cache_dirty( 0 );
    m.set_pathfinding_cache_dirty( 0 );
    m.build_map_cache( 0 );
}

static Character &place_player_at( const tripoint_bub_ms &p )
{
    Character &player = get_player_character();
    g->place_player( p );
    REQUIRE( player.pos_bub() == p );
    return player;
}

static void place_obstacle( map &m, const std::vector<tripoint_bub_ms> &places )
{
    const ter_id t_wall_metal( "t_wall_metal" );
    for( const tripoint_bub_ms &p : places ) {
        m.ter_set( p, t_wall_metal );
    }
    clear_map_caches( m );
    for( const tripoint_bub_ms &p : places ) {
        REQUIRE( !m.is_transparent( p ) );
    }
}

static void place_traps( map &m, const std::vector<tripoint_bub_ms> &places )
{
    const trap_id tr_beartrap( "tr_beartrap" );
    for( const tripoint_bub_ms &p : places ) {
        m.trap_set( p, tr_beartrap );
    }
    clear_map_caches( m );
}

static void place_wreckage( map &m, const std::vector<tripoint_bub_ms> &places )
{
    const furn_id f_wreckage( "f_wreckage" );
    for( const tripoint_bub_ms &p : places ) {
        m.furn_set( p, f_wreckage );
    }
    clear_map_caches( m );
}

static void place_fires( map &m, const std::vector<tripoint_bub_ms> &places )
{
    for( const tripoint_bub_ms &p : places ) {
        m.add_field( p, fd_fire, 1, 10_minutes );
    }
    clear_map_caches( m );
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


TEST_CASE( "map_route_player_without_obstacles", "[map][pathfinding]" )
{
    const map &m = setup_map_without_obstacles();
    const tripoint_bub_ms source = { 65, 65, 0 };
    const Character &pc = place_player_at( source );
    GIVEN( "Player on an empty map" ) {
        /*
         * Map layout:
         *   . . . . . .     @=player
         *   . @ 2 . 1 .     .=empty floor
         *   . . . . . .     1,2=targets
         */
        const tripoint_bub_ms t1 = { 68, 65, 0 };
        const tripoint_bub_ms t2 = { 66, 65, 0 };
        WHEN( "map::route does pathfinding to a tile 3 steps away" ) {
            const std::vector<tripoint_bub_ms> path = m.route( pc, pathfinding_target::point( t1 ) );
            THEN( "pathfinder finds a straight path" ) {
                const std::vector<tripoint_bub_ms> expected_path = {
                    { 66, 65, 0 }, { 67, 65, 0 }, { 68, 65, 0 }
                };
                CHECK( path == expected_path );
            }
        }
        WHEN( "map::route does pathfinding to an adjacent tile" ) {
            const std::vector<tripoint_bub_ms> path = m.route( pc, pathfinding_target::point( t2 ) );
            THEN( "pathfinder finds a path with only one step" ) {
                const std::vector<tripoint_bub_ms> expected_path = {
                    { 66, 65, 0 }
                };
                CHECK( path == expected_path );
            }
        }
        WHEN( "map::route does pathfinding to the same tile that player is already on" ) {
            const std::vector<tripoint_bub_ms> path = m.route( pc, pathfinding_target::point( source ) );
            THEN( "pathfinder returns an empty path" ) {
                CHECK( path.empty() );
            }
        }
    }
    clear_map();
}

TEST_CASE( "map_route_player_around_obstacles", "[map][pathfinding]" )
{
    map &m = setup_map_without_obstacles();
    const Character &pc = place_player_at( tripoint_bub_ms{ 65, 65, 0 } );
    GIVEN( "Map has obstacles between player and target" ) {
        /*
         * Map layout:
         *   . . @ . . .     @=player    .=empty floor
         *   . # # # # .     #=trap or sharp wreckage
         *   . t t t . .     t=tiles adjacent to target center
         *   . t T t . .     T=target center
         */
        const pathfinding_target t = pathfinding_target::adjacent(
                                         tripoint_bub_ms { 65, 68, 0 } );
        const std::vector<tripoint_bub_ms> obstacles = {
            { 64, 66, 0 }, { 65, 66, 0 }, { 66, 66, 0 }, { 67, 66, 0 }
        };
        const std::vector<tripoint_bub_ms> expected_path = {
            { 64, 65, 0 }, { 63, 66, 0 }, { 64, 67, 0 }
        };
        GIVEN( "Obstacles are traps" ) {
            place_traps( m, obstacles );
            WHEN( "map::route does pathfinding" ) {
                const std::vector<tripoint_bub_ms> path = m.route( pc, t );
                THEN( "route avoids the traps" ) {
                    CHECK( path == expected_path );
                }
            }
            m.clear_traps();
        }
        GIVEN( "Obstacles are sharp wreckage" ) {
            place_wreckage( m, obstacles );
            WHEN( "map::route does pathfinding" ) {
                const std::vector<tripoint_bub_ms> path = m.route( pc, t );
                THEN( "route avoids the sharp wreckage" ) {
                    CHECK( path == expected_path );
                }
            }
        }
    }

    GIVEN( "An obstacle course between player and target" ) {
        /*
         * Map layout:
         * # # # # # # . . .
         * . . . . . # . # .    #=obstacle
         * . . . # . # . # .    @=player
         * . . @ # . . . # 1    1=target
         * # # # # # # # # .
         */
        place_obstacle( m, {
            { 63, 62, 0 }, { 63, 66, 0 },
            { 64, 62, 0 }, { 64, 66, 0 },
            { 65, 62, 0 }, { 65, 66, 0 },
            { 66, 62, 0 }, { 66, 64, 0 }, { 66, 65, 0 }, { 66, 66, 0 },
            { 67, 62, 0 }, { 67, 66, 0 },
            { 68, 62, 0 }, { 68, 63, 0 }, { 68, 64, 0 }, { 68, 66, 0 },
            { 69, 66, 0 },
            { 70, 63, 0 }, { 70, 64, 0 }, { 70, 65, 0 }, { 70, 66, 0 },
        } );
        const tripoint_bub_ms target1{ 71, 65, 0 };
        WHEN( "map::route does pathfinding of player to target" ) {
            const std::vector<tripoint_bub_ms> path =
                m.route( pc, pathfinding_target::point( target1 ) );
            THEN( "path avoids the obstacles and ends at target" ) {
                CHECK( path[0] == tripoint_bub_ms{ 65, 64, 0 } );
                CHECK( path[1] == tripoint_bub_ms{ 66, 63, 0 } );
                CHECK( path[2] == tripoint_bub_ms{ 67, 64, 0 } );
                CHECK( path[3] == tripoint_bub_ms{ 68, 65, 0 } );
                CHECK( path[4] == tripoint_bub_ms{ 69, 64, 0 } );
                CHECK( path[5] == tripoint_bub_ms{ 69, 63, 0 } );
                CHECK( path[6] == tripoint_bub_ms{ 70, 62, 0 } );
                CHECK( path[7] == tripoint_bub_ms{ 71, 63, 0 } );
                CHECK( path[8] == tripoint_bub_ms{ 71, 64, 0 } );
                CHECK( path[9] == tripoint_bub_ms{ 71, 65, 0 } );
                CHECK( path.size() == 10 );
            }
        }

        const tripoint_bub_ms target2{ 68, 64, 0 };
        REQUIRE( !m.is_transparent( target2 ) );
        WHEN( "map::route does pathfinding of player to a target tile that is not walkable, with target radius 2" ) {
            const std::vector<tripoint_bub_ms> path =
                m.route( pc, pathfinding_target::radius( target2, 2 ) );
            THEN( "path avoids the obstacles and ends 2 tiles from target" ) {
                const std::vector<tripoint_bub_ms> expected_path = {
                    { 65, 64, 0 }, { 66, 63, 0 }
                };
                CHECK( path == expected_path );
            }
        }
    }
    clear_map();
}

TEST_CASE( "map_route_player_into_danger", "[map][pathfinding]" )
{
    map &m = setup_map_without_obstacles();
    const Character &pc = place_player_at( tripoint_bub_ms{ 65, 65, 0 } );
    GIVEN( "Map has obstacles around player, and target is not reachable by flat ground" ) {
        /*
         * Map layout:
         *   . # # # .     #=trap or sharp wreckage
         *   . # @ # .     @=player
         *   . # # # .
         *   . . . . .     .=empty floor
         *   . . t . .     t=target
         */
        const pathfinding_target t = pathfinding_target::point(
                                         tripoint_bub_ms { 65, 68, 0 } );
        const std::vector<tripoint_bub_ms> obstacles = {
            { 64, 64, 0 }, { 65, 64, 0 }, { 66, 64, 0 },
            { 64, 65, 0 },                { 66, 65, 0 },
            { 64, 66, 0 }, { 65, 66, 0 }, { 66, 66, 0 }
        };
        const std::vector<tripoint_bub_ms> expected_path = {
            { 65, 66, 0 }, { 65, 67, 0 }, { 65, 68, 0 }
        };
        GIVEN( "Obstacles are traps" ) {
            place_traps( m, obstacles );
            WHEN( "map::route does pathfinding" ) {
                const std::vector<tripoint_bub_ms> path = m.route( pc, t );
                THEN( "route goes through the traps" ) {
                    CHECK( path == expected_path );
                }
            }
            m.clear_traps();
        }
        GIVEN( "Obstacles are sharp wreckage" ) {
            place_wreckage( m, obstacles );
            WHEN( "map::route does pathfinding" ) {
                const std::vector<tripoint_bub_ms> path = m.route( pc, t );
                THEN( "route goes through the sharp wreckage" ) {
                    CHECK( path == expected_path );
                }
            }
        }
    }
    clear_map();
}

TEST_CASE( "map_route_player_up_down_stairs", "[map][pathfinding]" )
{
    map &m = setup_map_without_obstacles();
    const ter_id t_stairs_up( "t_stairs_up" );
    const ter_id t_stairs_down( "t_stairs_down" );
    const ter_id t_floor( "t_floor" );
    const Character &pc = place_player_at( tripoint_bub_ms{ 65, 65, 0 } );
    GIVEN( "Map with stairs going up" ) {
        /*
         * Map layout:
         *   . . . . . . .    .=empty floor
         *   . @ . < . t .    @=player      <=stairs going up
         *   . . . . . . .    t=target (one z-level above)
         */
        m.ter_set( tripoint_bub_ms{ 67, 65, 0 }, t_stairs_up );
        m.ter_set( tripoint_bub_ms{ 67, 65, 1 }, t_stairs_down );
        clear_map_caches( m );
        WHEN( "map::route does pathfinding" ) {
            const pathfinding_target t = pathfinding_target::point(
                                             tripoint_bub_ms { 69, 65, 1 } );
            const std::vector<tripoint_bub_ms> path = m.route( pc, t );
            THEN( "route goes up the stairs" ) {
                const std::vector<tripoint_bub_ms> expected_path = {
                    { 66, 65, 0 }, { 67, 65, 0 }, { 67, 65, 1 },
                    { 68, 65, 1 }, { 69, 65, 1 }
                };
                CHECK( path == expected_path );
            }
        }
    }
    GIVEN( "Map with stairs going down" ) {
        /*
         * Map layout:
         *   . . . . . . .    .=empty floor
         *   . @ . > . t .    @=player      >=stairs going down
         *   . . . . . . .    t=target (one z-level below)
         */
        m.ter_set( tripoint_bub_ms{ 67, 65, 0 }, t_stairs_down );
        m.ter_set( tripoint_bub_ms{ 67, 65, -1 }, t_stairs_up );
        m.ter_set( tripoint_bub_ms{ 68, 65, -1 }, t_floor );
        m.ter_set( tripoint_bub_ms{ 69, 65, -1 }, t_floor );
        clear_map_caches( m );
        WHEN( "map::route does pathfinding" ) {
            const pathfinding_target t = pathfinding_target::point(
                                             tripoint_bub_ms { 69, 65, -1 } );
            const std::vector<tripoint_bub_ms> path = m.route( pc, t );
            THEN( "route goes down the stairs" ) {
                const std::vector<tripoint_bub_ms> expected_path = {
                    { 66, 65, 0 }, { 67, 65, 0 }, { 67, 65, -1 },
                    { 68, 65, -1 }, { 69, 65, -1 }
                };
                CHECK( path == expected_path );
            }
        }
    }
    clear_map();
}

TEST_CASE( "map_route_player_into_unreachable_tiles", "[map][pathfinding]" )
{
    map &m = setup_map_without_obstacles();
    const Character &pc = place_player_at( tripoint_bub_ms{ 65, 65, 0 } );
    const tripoint_bub_ms not_passable = { 68, 65, 0 };
    GIVEN( "Map has unpassable tiles as target" ) {
        /*
         * Map layout:
         *   . . . . . .    .=empty floor
         *   . @ . . # .    @=player
         *   . . . . . .    #=solid wall, is also the target
         */
        place_obstacle( m, { not_passable } );
        WHEN( "map::route does pathfinding to a target tile that is not passable" ) {
            const std::vector<tripoint_bub_ms> path = m.route( pc, pathfinding_target::point( not_passable ) );
            THEN( "it does not find any route" ) {
                CHECK( path.empty() );
            }
        }
        WHEN( "map::route does pathfinding to any adjacent tile of an unpassable tile" ) {
            const std::vector<tripoint_bub_ms> path = m.route( pc,
                    pathfinding_target::adjacent( not_passable ) );
            THEN( "route ends on one of the adjacent tiles" ) {
                const std::vector<tripoint_bub_ms> expected_path = {
                    { 66, 65, 0 }, { 67, 65, 0 }
                };
                CHECK( path == expected_path );
            }
        }
    }
    GIVEN( "Player is surrounded by walls" ) {
        /*
         * Map layout:
         *   # # # . . .    .=empty floor
         *   # @ # . t .    @=player     t=target
         *   # # # . . .    #=solid wall
         */
        const std::vector<tripoint_bub_ms> obstacles = {
            { 64, 64, 0 }, { 65, 64, 0 }, { 66, 64, 0 },
            { 64, 65, 0 },                { 66, 65, 0 },
            { 64, 66, 0 }, { 65, 66, 0 }, { 66, 66, 0 }
        };
        place_obstacle( m, obstacles );
        WHEN( "map::route does pathfinding" ) {
            const std::vector<tripoint_bub_ms> path = m.route( pc,
                    pathfinding_target::adjacent( not_passable ) );
            THEN( "it does not find any route" ) {
                CHECK( path.empty() );
            }
        }
    }
    clear_map();
}

TEST_CASE( "map_route_mon_around_danger", "[map][pathfinding]" )
{
    map &m = setup_map_without_obstacles();
    const monster &mon = spawn_test_monster( "mon_mi_go", tripoint_bub_ms{ 10, 10, 0 } );
    GIVEN( "Map has obstacles between mi-go and target" ) {
        /*
         * Map layout:
         *   . # # # . .     #=trap, sharp wreckage or fire
         *   . # & # . t     &=mi-go
         *   . # . # . .     t=target
         *   . . . . . .
         */
        const pathfinding_target t = pathfinding_target::point(
                                         tripoint_bub_ms { 13, 10, 0 } );
        const std::vector<tripoint_bub_ms> obstacles = {
            { 9, 9,  0 }, { 10, 9,  0 }, { 11, 9,  0 },
            { 9, 10, 0 },                { 11, 10, 0 },
            { 9, 11, 0 },                { 11, 11, 0 }
        };
        const std::vector<tripoint_bub_ms> expected_path = {
            { 10, 11, 0 }, { 11, 12, 0 }, { 12, 11, 0 }, { 13, 10, 0 }
        };
        GIVEN( "Obstacles are traps" ) {
            place_traps( m, obstacles );
            WHEN( "map::route does pathfinding for mi-go" ) {
                const std::vector<tripoint_bub_ms> path = m.route( mon, t );
                THEN( "route avoids the traps" ) {
                    CHECK( path == expected_path );
                }
            }
            m.clear_traps();
        }
        GIVEN( "Obstacles are sharp wreckage" ) {
            place_wreckage( m, obstacles );
            WHEN( "map::route does pathfinding for mi-go" ) {
                const std::vector<tripoint_bub_ms> path = m.route( mon, t );
                THEN( "route avoids the sharp wreckage" ) {
                    CHECK( path == expected_path );
                }
            }
            for( const tripoint_bub_ms &p : obstacles ) {
                m.furn_clear( p );
            }
        }
        GIVEN( "Obstacles are fire" ) {
            place_fires( m, obstacles );
            WHEN( "map::route does pathfinding for mi-go" ) {
                const std::vector<tripoint_bub_ms> path = m.route( mon, t );
                THEN( "route avoids the fire" ) {
                    CHECK( path == expected_path );
                }
            }
            for( const tripoint_bub_ms &p : obstacles ) {
                m.clear_fields( p );
            }
        }
    }
    clear_map();
}
