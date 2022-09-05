#include "cata_catch.h"

#if (defined(TILES))

#include "avatar.h"
#include "enums.h"
#include "map.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "sdltiles.h"

#define protected public
#include "cata_tiles.h"


TEST_CASE( "walls should connect to walls", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    tripoint pos = get_avatar().pos() + point_east + point_east;

    int subtile = 0;
    int rotation = 0;

    // Unconnected
    WHEN( "no connecting neighbours" ) {
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );

        THEN( "the wall should be unconnected" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == unconnected );
            CHECK( rotation % 1 == 0 ); // 16 possible rotations due to rotates_to
        }
    }
    // End pieces
    WHEN( "connecting neighbour south" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as end_piece N" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == end_piece );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour east" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as end_piece W" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == end_piece );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbour north" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as end_piece S" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == end_piece );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbour west" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as end_piece E" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == end_piece );
            CHECK( rotation == 3 );
        }
    }
    // Corners
    WHEN( "connecting neighbour south and east" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as corner NW" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == corner );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour north and east" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as corner SW" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == corner );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbour north and west" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as corner SE" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == corner );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbour south and west" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as corner NE" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == corner );
            CHECK( rotation == 3 );
        }
    }
    // Edges
    WHEN( "connecting neighbour north and south" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as edge NS" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == edge );
            CHECK( rotation % 2 == 0 ); // 8 possible rotations due to rotates_to
        }
    }
    WHEN( "connecting neighbour east and west" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as edge EW" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == edge );
            CHECK( rotation % 2 == 1 ); // 8 possible rotations due to rotates_to
        }
    }
    // T connections
    WHEN( "connecting neighbour all but north" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as t-connection N" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == t_connection );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour all but west" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as t-connection W" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == t_connection );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbour all but south" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as t-connection S" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == t_connection );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbour all but east" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as t-connection E" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == t_connection );
            CHECK( rotation == 3 );
        }
    }
    // All
    WHEN( "connecting neighbour all" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as center" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_NONE, {} );
            CHECK( subtile == center );
            CHECK( rotation == 0 );
        }
    }
}

TEST_CASE( "windows should connect to walls and rotate to indoor floor", "[multitile][rotates]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    tripoint pos = get_avatar().pos() + point_east + point_east;

    int subtile = 0;
    int rotation = 0;

    // Edges
    WHEN( "connecting neighbours north and south, and rotate to east" ) {
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with E positive" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to south" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window should be connected as EW, with S positive" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbours north and south, and rotate to west" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with W positive" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to north" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );

        THEN( "the window should be connected EW, with N positive" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 3 );
        }
    }

    WHEN( "connecting neighbours north and south, and nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with E and W negative" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 4 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to north and south" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );

        THEN( "the window should be connected as EW, with N and S positive" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 5 );
        }
    }
    WHEN( "connecting neighbours north and south, and rotate to east and west" ) {
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with E and W negative" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 6 );
        }
    }
    WHEN( "connecting neighbours east and west, and nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window should be connected as EW, with N and S negative" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == edge );
            CHECK( rotation == 7 );
        }
    }
}

TEST_CASE( "unconnected windows rotate to indoor floor", "[multitile][rotates]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    tripoint pos = get_avatar().pos() + point_east + point_east;

    int subtile = 0;
    int rotation = 0;

    // Unconnected
    WHEN( "nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window should be unconnected" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == unconnected );
            CHECK( rotation == 14 );
        }
    }

    WHEN( "indoor floor to the north" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );

        THEN( "the window rotate to the north" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == unconnected );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "indoor floor to the east" ) {
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window rotate to the east" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == unconnected );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "indoor floor to the south" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window rotate to the south" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == unconnected );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "indoor floor to the west" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window rotate to the west" ) {
            tilecontext->get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                                             TERCONN_INDOORFLOOR, {} );
            CHECK( subtile == unconnected );
            CHECK( rotation == 3 );
        }
    }
}

#endif // SDL_TILES
