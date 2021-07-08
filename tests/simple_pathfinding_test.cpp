#include "cata_catch.h"
#include "simple_pathfinding.h"

#include "coordinates.h"
#include "point.h"

template<typename Point>
static void test_path()
{
    Point start;
    Point finish( point( 3, 0 ) );
    Point max( point( 10, 10 ) );

    const auto estimate =
    [&]( const pf::node<Point> &, const pf::node<Point> * ) {
        return 1;
    };

    pf::path<Point> pth = pf::find_path( start, finish, max, estimate );
    REQUIRE( pth.nodes.size() == 4 );
    CHECK( pth.nodes[3].pos == Point() );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    CHECK( pth.nodes[2].pos == Point( point( 1, 0 ) ) );
    CHECK( pth.nodes[1].pos == Point( point( 2, 0 ) ) );
    CHECK( pth.nodes[0].pos == Point( point( 3, 0 ) ) );
}

TEST_CASE( "simple_line_path" )
{
    test_path<point>();
    test_path<point_abs_omt>();
}
