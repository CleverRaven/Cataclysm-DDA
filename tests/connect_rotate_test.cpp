#if (defined(TILES))

#include "avatar.h"
#include "cata_catch.h"
#include "cata_tiles.h"
#include "enums.h"
#include "map.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "sdltiles.h"

class cata_tiles_test_helper
{
    public:
        static void get_connect_values( const tripoint &p, int &subtile, int &rotation,
                                        int connect_group, int rotate_to_group ) {
            cata_tiles::get_connect_values( p, subtile, rotation, connect_group, rotate_to_group, {} );
        }
};

TEST_CASE( "walls should be unconnected without nearby walls", "[multitile][connects]" )
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
            CHECK( subtile == unconnected );
            CHECK( rotation == 0 );
        }
    }
}
TEST_CASE( "walls should connect to walls as end pieces", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    tripoint pos = get_avatar().pos() + point_east + point_east;

    int subtile = 0;
    int rotation = 0;

    // End pieces
    WHEN( "connecting neighbour south" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as end_piece N" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
            CHECK( subtile == end_piece );
            CHECK( rotation == 3 );
        }
    }
}
TEST_CASE( "walls should connect to walls as corners", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    tripoint pos = get_avatar().pos() + point_east + point_east;

    int subtile = 0;
    int rotation = 0;

    // Corners
    WHEN( "connecting neighbour south and east" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as corner NW" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
            CHECK( subtile == corner );
            CHECK( rotation == 3 );
        }
    }
}
TEST_CASE( "walls should connect to walls as edges", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    tripoint pos = get_avatar().pos() + point_east + point_east;

    int subtile = 0;
    int rotation = 0;

    // Edges
    WHEN( "connecting neighbour north and south" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );

        THEN( "the wall should be connected as edge NS" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
            CHECK( subtile == edge );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbour east and west" ) {
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as edge EW" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
            CHECK( subtile == edge );
            CHECK( rotation == 1 );
        }
    }
}
TEST_CASE( "walls should connect to walls as t-connections and fully", "[multitile][connects]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();

    tripoint pos = get_avatar().pos() + point_east + point_east;

    int subtile = 0;
    int rotation = 0;

    // T connections
    WHEN( "connecting neighbour all but north" ) {
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );

        THEN( "the wall should be connected as t-connection N" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_NONE );
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
    WHEN( "connecting neighbours north and south, and rotate to west" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with W positive" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == edge );
            CHECK( rotation == 0 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to north" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );

        THEN( "the window should be connected EW, with N positive" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == edge );
            CHECK( rotation == 1 );
        }
    }
    WHEN( "connecting neighbours north and south, and rotate to east" ) {
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with E positive" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == edge );
            CHECK( rotation == 2 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to south" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window should be connected as EW, with S positive" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == edge );
            CHECK( rotation == 3 );
        }
    }

    WHEN( "connecting neighbours north and south, and rotate to east and west" ) {
        REQUIRE( here.ter_set( pos + point_east, t_floor ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_floor ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with E and W negative" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == edge );
            CHECK( rotation == 4 );
        }
    }
    WHEN( "connecting neighbours east and west, and nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_pavement ) );

        THEN( "the window should be connected as EW, with N and S negative" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == edge );
            CHECK( rotation == 5 );
        }
    }
    WHEN( "connecting neighbours north and south, and nothing to rotate to" ) {
        REQUIRE( here.ter_set( pos + point_east, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_south, t_wall ) );
        REQUIRE( here.ter_set( pos + point_west, t_pavement ) );
        REQUIRE( here.ter_set( pos + point_north, t_wall ) );

        THEN( "the window should be connected as NS, with E and W negative" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == edge );
            CHECK( rotation == 6 );
        }
    }
    WHEN( "connecting neighbours east and west, and rotate to north and south" ) {
        REQUIRE( here.ter_set( pos + point_east, t_wall ) );
        REQUIRE( here.ter_set( pos + point_south, t_floor ) );
        REQUIRE( here.ter_set( pos + point_west, t_wall ) );
        REQUIRE( here.ter_set( pos + point_north, t_floor ) );

        THEN( "the window should be connected as EW, with N and S positive" ) {
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_WALL,
                    TERCONN_INDOORFLOOR );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                    TERCONN_INDOORFLOOR );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                    TERCONN_INDOORFLOOR );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                    TERCONN_INDOORFLOOR );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                    TERCONN_INDOORFLOOR );
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
            cata_tiles_test_helper::get_connect_values( pos, subtile, rotation, TERCONN_NONE,
                    TERCONN_INDOORFLOOR );
            CHECK( subtile == unconnected );
            CHECK( rotation == 3 );
        }
    }
}

#endif // SDL_TILES
