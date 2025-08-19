#if (defined(TILES))

#include <bitset>
#include <map>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "cata_tiles.h"
#include "coordinates.h"
#include "enums.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_pavement( "t_pavement" );
static const ter_str_id ter_t_wall( "t_wall" );

class cata_tiles_test_helper
{
    public:
        static void get_connect_values( const tripoint &p, int &subtile, int &rotation,
                                        const std::bitset<NUM_TERCONN> &connect_group,
                                        const std::bitset<NUM_TERCONN> &rotate_to_group ) {
            cata_tiles::get_connect_values( tripoint_bub_ms( p ), subtile, rotation, connect_group,
                                            rotate_to_group, {} );
        }
};

TEST_CASE( "walls_should_be_unconnected_without_nearby_walls", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    std::bitset<NUM_TERCONN> none;
    std::bitset<NUM_TERCONN> wall;
    wall.set( get_connect_group( "WALL" ).index );

    tripoint_bub_ms pos = get_avatar().pos_bub() + point::east + point::east;

    int subtile = 0;
    int rotation = 0;

    // Unconnected
    WHEN( "no connecting neighbours" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );

        THEN( "the wall should be unconnected" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == unconnected );
            CHECK( rotation == 0 );
        }
    }
}
TEST_CASE( "walls_should_connect_to_walls_as_end_pieces", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    std::bitset<NUM_TERCONN> none;
    std::bitset<NUM_TERCONN> wall;
    wall.set( get_connect_group( "WALL" ).index );

    tripoint_bub_ms pos = get_avatar().pos_bub() + point::east + point::east;

    int subtile = 0;
    int rotation = 0;

    // End pieces
    WHEN( "connecting neighbour south" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );

        THEN( "the wall should be connected as end_piece N" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == end_piece );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour east" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );

        THEN( "the wall should be connected as end_piece W" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == end_piece );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbour north" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );

        THEN( "the wall should be connected as end_piece S" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == end_piece );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbour west" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as end_piece E" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == end_piece );
            CHECK( rotation == 3 );
        }
    }
}
TEST_CASE( "walls_should_connect_to_walls_as_corners", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    std::bitset<NUM_TERCONN> none;
    std::bitset<NUM_TERCONN> wall;
    wall.set( get_connect_group( "WALL" ).index );

    tripoint_bub_ms pos = get_avatar().pos_bub() + point::east + point::east;

    int subtile = 0;
    int rotation = 0;

    // Corners
    WHEN( "connecting neighbour south and east" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );

        THEN( "the wall should be connected as corner NW" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == corner );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour north and east" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );

        THEN( "the wall should be connected as corner SW" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == corner );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbour north and west" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as corner SE" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == corner );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbour south and west" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as corner NE" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == corner );
            CHECK( rotation == 3 );
        }
    }
}
TEST_CASE( "walls_should_connect_to_walls_as_edges", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    std::bitset<NUM_TERCONN> none;
    std::bitset<NUM_TERCONN> wall;
    wall.set( get_connect_group( "WALL" ).index );

    tripoint_bub_ms pos = get_avatar().pos_bub() + point::east + point::east;

    int subtile = 0;
    int rotation = 0;

    // Edges
    WHEN( "connecting neighbour north and south" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );

        THEN( "the wall should be connected as edge NS" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == edge );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour east and west" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as edge EW" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == edge );
            CHECK( rotation == 1 );
        }
    }
}
TEST_CASE( "walls_should_connect_to_walls_as_t-connections_and_fully", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    std::bitset<NUM_TERCONN> none;
    std::bitset<NUM_TERCONN> wall;
    wall.set( get_connect_group( "WALL" ).index );

    tripoint_bub_ms pos = get_avatar().pos_bub() + point::east + point::east;

    int subtile = 0;
    int rotation = 0;

    // T connections
    WHEN( "connecting neighbour all but north" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as t-connection N" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == t_connection );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour all but west" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );

        THEN( "the wall should be connected as t-connection W" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == t_connection );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbour all but south" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as t-connection S" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == t_connection );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbour all but east" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as t-connection E" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == t_connection );
            CHECK( rotation == 3 );
        }
    }
    // All
    WHEN( "connecting neighbour all" ) {
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );

        THEN( "the wall should be connected as center" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, none );
            CHECK( subtile == center );
            CHECK( rotation == 0 );
        }
    }
}

TEST_CASE( "windows_should_connect_to_walls_and_rotate_to_indoor_floor", "[multitile][rotates]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    std::bitset<NUM_TERCONN> floor;
    floor.set( get_connect_group( "INDOORFLOOR" ).index );
    std::bitset<NUM_TERCONN> wall;
    wall.set( get_connect_group( "WALL" ).index );

    tripoint_bub_ms pos = get_avatar().pos_bub() + point::east + point::east;

    int subtile = 0;
    int rotation = 0;

    // Edges
    WHEN( "connecting neighbours north and south, and rotate to west" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );

        THEN( "the window should be connected as NS, with W positive" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to north" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );

        THEN( "the window should be connected EW, with N positive" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 3 );
        }
    }
    WHEN( "connecting neighbours north and south, and rotate to east" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );

        THEN( "the window should be connected as NS, with E positive" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to south" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_pavement ) );

        THEN( "the window should be connected as EW, with S positive" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 1 );
        }
    }

    WHEN( "connecting neighbours north and south, and rotate to east and west" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );

        THEN( "the window should be connected as NS, with E and W negative" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 4 );
        }
    }
    WHEN( "connecting neighbours east and west, and nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_pavement ) );

        THEN( "the window should be connected as EW, with N and S negative" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 7 );
        }
    }
    WHEN( "connecting neighbours north and south, and nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_wall ) );

        THEN( "the window should be connected as NS, with E and W negative" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 6 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to north and south" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_wall ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );

        THEN( "the window should be connected as EW, with N and S positive" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    wall, floor );
            CHECK( subtile == edge );
            CHECK( rotation == 5 );
        }
    }
}

TEST_CASE( "unconnected_windows_rotate_to_indoor_floor", "[multitile][rotates]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    std::bitset<NUM_TERCONN> none;
    std::bitset<NUM_TERCONN> floor;
    floor.set( get_connect_group( "INDOORFLOOR" ).index );

    tripoint_bub_ms pos = get_avatar().pos_bub() + point::east + point::east;

    int subtile = 0;
    int rotation = 0;

    // Unconnected
    WHEN( "nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_pavement ) );

        THEN( "the window should be unconnected" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    none, floor );
            CHECK( subtile == unconnected );
            CHECK( rotation == 15 );
        }
    }

    WHEN( "indoor floor to the north" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_floor ) );

        THEN( "the window rotate to the north" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    none, floor );
            CHECK( subtile == unconnected );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "indoor floor to the east" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_pavement ) );

        THEN( "the window rotate to the east" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    none, floor );
            CHECK( subtile == unconnected );
            CHECK( rotation == 3 );
        }
    }
    WHEN( "indoor floor to the south" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_pavement ) );

        THEN( "the window rotate to the south" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    none, floor );
            CHECK( subtile == unconnected );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "indoor floor to the west" ) {
        REQUIRE( here.ter_set( pos + point::east, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::south, ter_t_pavement ) );
        REQUIRE( here.ter_set( pos + point::west, ter_t_floor ) );
        REQUIRE( here.ter_set( pos + point::north, ter_t_pavement ) );

        THEN( "the window rotate to the west" ) {
            cata_tiles_test_helper::get_connect_values( pos.raw(), subtile, rotation,
                    none, floor );
            CHECK( subtile == unconnected );
            CHECK( rotation == 1 );
        }
    }
}

#endif // SDL_TILES
