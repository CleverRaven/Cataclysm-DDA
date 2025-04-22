#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include "cata_catch.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "line.h"
#include "omdata.h"
#include "point.h"
#include "simple_pathfinding.h"

template<typename Point>
static void test_greedy_line_path()
{
    const Point start( 0, 0 ); // NOLINT(cata-use-named-point-constants,cata-point-initialization)
    const Point finish( 3, 0 );
    const Point max( 10, 10 );

    const pf::two_node_scoring_fn<Point> estimate =
    [&]( pf::directed_node<Point> cur, std::optional<pf::directed_node<Point>> ) {
        return pf::node_score( 0, manhattan_dist( cur.pos, finish ) );
    };

    const pf::directed_path<Point> pth = pf::greedy_path( start, finish, max, estimate );
    REQUIRE( pth.nodes.size() == 4 );
    CHECK( pth.nodes[3].pos == Point( 0, 0 ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( pth.nodes[2].pos == Point( 1, 0 ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( pth.nodes[1].pos == Point( 2, 0 ) );
    CHECK( pth.nodes[0].pos == Point( 3, 0 ) );
}

TEST_CASE( "greedy_simple_line_path", "[pathfinding]" )
{
    test_greedy_line_path<point>();
    test_greedy_line_path<point_abs_omt>();
}

template<typename Point>
static void test_greedy_u_bend()
{
    const Point start( 0, 0 ); // NOLINT(cata-use-named-point-constants,cata-point-initialization)
    const Point finish( 2, 0 );
    const Point max( 3, 3 );
    // Test area and expected path:
    // SxF    6x0
    // .x.    5x1
    // ...    432

    const pf::two_node_scoring_fn<Point> estimate =
    [&]( pf::directed_node<Point> cur, std::optional<pf::directed_node<Point>> ) {
        if( cur.pos.x() == 1 && cur.pos.y() != 2 ) {
            return pf::node_score::rejected;
        }
        return pf::node_score( 0, manhattan_dist( cur.pos, finish ) );
    };

    const pf::directed_path<Point> pth = pf::greedy_path( start, finish, max, estimate );
    REQUIRE( pth.nodes.size() == 7 );
    CHECK( pth.nodes[6].pos == Point( 0, 0 ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( pth.nodes[6].dir == om_direction::type::invalid );
    CHECK( pth.nodes[5].pos == Point( 0, 1 ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( pth.nodes[5].dir == om_direction::type::north );
    CHECK( pth.nodes[4].pos == Point( 0, 2 ) );
    CHECK( pth.nodes[4].dir == om_direction::type::north );
    CHECK( pth.nodes[3].pos == Point( 1, 2 ) );
    CHECK( pth.nodes[3].dir == om_direction::type::west );
    CHECK( pth.nodes[2].pos == Point( 2, 2 ) );
    CHECK( pth.nodes[2].dir == om_direction::type::west );
    CHECK( pth.nodes[1].pos == Point( 2, 1 ) );
    CHECK( pth.nodes[1].dir == om_direction::type::south );
    CHECK( pth.nodes[0].pos == Point( 2, 0 ) );
    CHECK( pth.nodes[0].dir == om_direction::type::south );
}

TEST_CASE( "greedy_u_bend", "[pathfinding]" )
{
    test_greedy_u_bend<point_om_omt>();
}

static std::function<void( size_t, size_t )> noop_fn = []( size_t, size_t ) {};

TEST_CASE( "find_overmap_path_u_bend", "[pathfinding]" )
{
    using Point = tripoint_abs_omt;
    const Point start( 0, 0, 0 );
    const Point finish( 2, 0, 0 );
    const inclusive_cuboid<Point> bounds( start, Point( 2, 2, 0 ) );
    // Test area and expected path:
    // SxF    6x0
    // .x.    5x1
    // ...    432

    const pf::omt_scoring_fn estimate = [&]( Point cur ) {
        if( !bounds.contains( cur ) || ( cur.x() == 1 && cur.y() != 2 ) ) {
            return pf::omt_score::rejected;
        }
        return pf::omt_score( 10, false );
    };

    const pf::simple_path<Point> pth = pf::find_overmap_path( start, finish, 2, estimate, noop_fn );
    REQUIRE( pth.points.size() == 7 );
    CHECK( pth.points[6] == Point( 0, 0, 0 ) );
    CHECK( pth.points[5] == Point( 0, 1, 0 ) );
    CHECK( pth.points[4] == Point( 0, 2, 0 ) );
    CHECK( pth.points[3] == Point( 1, 2, 0 ) );
    CHECK( pth.points[2] == Point( 2, 2, 0 ) );
    CHECK( pth.points[1] == Point( 2, 1, 0 ) );
    CHECK( pth.points[0] == Point( 2, 0, 0 ) );
}

TEST_CASE( "find_overmap_path_bridge", "[pathfinding]" )
{
    using Point = tripoint_abs_omt;
    const Point start( 0, 0, 0 );
    const Point finish( 2, 0, 0 );
    const inclusive_cuboid<Point> bounds( start, Point( 2, 2, 1 ) );
    // Test area and expected path:
    // SxF    6x0
    // ^x^    5x1
    // .x.    .x.
    // ( points 2, 3, 4 are at z=1 )

    const pf::omt_scoring_fn estimate = [&]( Point cur ) {
        if( !bounds.contains( cur ) || ( cur.x() == 1 && cur.z() == 0 ) ) {
            return pf::omt_score::rejected;
        }
        return pf::omt_score( 10, ( cur.y() == 1 && cur.x() != 1 ) );
    };

    const pf::simple_path<Point> pth = pf::find_overmap_path( start, finish, 2, estimate, noop_fn );
    REQUIRE( pth.points.size() == 7 );
    CHECK( pth.points[6] == Point( 0, 0, 0 ) );
    CHECK( pth.points[5] == Point( 0, 1, 0 ) );
    CHECK( pth.points[4] == Point( 0, 1, 1 ) );
    CHECK( pth.points[3] == Point( 1, 1, 1 ) );
    CHECK( pth.points[2] == Point( 2, 1, 1 ) );
    CHECK( pth.points[1] == Point( 2, 1, 0 ) );
    CHECK( pth.points[0] == Point( 2, 0, 0 ) );
}

