#include "cata_catch.h"
#include "simple_pathfinding.h"

#include "coordinates.h"
#include "point.h"

template<typename Point>
static void test_path()
{
    const Point start( 0, 0 );
    const Point finish( 3, 0 );
    const Point max( 10, 10 );

    const auto estimate =
    [&]( const pf::node<Point> &, const pf::node<Point> * ) {
        return 1;
    };

    pf::path<Point> pth = pf::find_path( start, finish, max, estimate );
    REQUIRE( pth.nodes.size() == 4 );
    CHECK( pth.nodes[3].pos == Point( 0, 0 ) );
    CHECK( pth.nodes[2].pos == Point( 1, 0 ) );
    CHECK( pth.nodes[1].pos == Point( 2, 0 ) );
    CHECK( pth.nodes[0].pos == Point( 3, 0 ) );
}

TEST_CASE( "simple_line_path" )
{
    test_path<point>();
    test_path<point_abs_omt>();
}

template<typename Point>
static void test_greedy_u_bend()
{
    const Point start( 0, 0 );
    const Point finish( 2, 0 );
    const Point max( 3, 3 );
    // Test area and expected path:
    // SxF    6x0
    // .x.    5x1
    // ...    432

    const auto estimate =
    [&]( const pf::node<Point> &cur, const pf::node<Point>* ) {
        if( cur.pos.x() == 1 && cur.pos.y() != 2) {
            return pf::rejected;
        }
        return manhattan_dist( cur.pos, finish );
    };

    const pf::path<Point> pth = pf::find_path( start, finish, max, estimate );
    REQUIRE( pth.nodes.size() == 7 );
    CHECK( pth.nodes[6].pos == Point( 0, 0 ) );
    CHECK( pth.nodes[6].dir == -1 /* invalid */ );
    CHECK( pth.nodes[5].pos == Point( 0, 1 ) );
    CHECK( pth.nodes[5].dir == 0 /* north */ );
    CHECK( pth.nodes[4].pos == Point( 0, 2 ) );
    CHECK( pth.nodes[4].dir == 0 /* north */ );
    CHECK( pth.nodes[3].pos == Point( 1, 2 ) );
    CHECK( pth.nodes[3].dir == 3 /* west */ );
    CHECK( pth.nodes[2].pos == Point( 2, 2 ) );
    CHECK( pth.nodes[2].dir == 3 /* west */ );
    CHECK( pth.nodes[1].pos == Point( 2, 1 ) );
    CHECK( pth.nodes[1].dir == 2 /* south */ );
    CHECK( pth.nodes[0].pos == Point( 2, 0 ) );
    CHECK( pth.nodes[0].dir == 2 /* south */ );
}

TEST_CASE( "greedy_u_bend" )
{
    test_greedy_u_bend<point_om_omt>();
}
