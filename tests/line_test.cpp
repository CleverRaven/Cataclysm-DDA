#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <vector>

#include "catch/catch.hpp"
#include "line.h"
#include "point.h"
#include "rng.h"

#define SGN(a) (((a)<0) ? -1 : 1)
// Compare all future line_to implementations to the canonical one.
static std::vector <point> canonical_line_to( const point &p1, const point &p2, int t )
{
    std::vector<point> ret;
    const int dx = p2.x - p1.x;
    const int dy = p2.y - p1.y;
    const int ax = std::abs( dx ) << 1;
    const int ay = std::abs( dy ) << 1;
    int sx = SGN( dx );
    int sy = SGN( dy );
    if( dy == 0 ) {
        sy = 0;
    }
    if( dx == 0 ) {
        sx = 0;
    }
    point cur;
    cur.x = p1.x;
    cur.y = p1.y;

    int xmin = std::min( p1.x, p2.x );
    int ymin = std::min( p1.y, p2.y );
    int xmax = std::max( p1.x, p2.x );
    int ymax = std::max( p1.y, p2.y );

    xmin -= std::abs( dx );
    ymin -= std::abs( dy );
    xmax += std::abs( dx );
    ymax += std::abs( dy );

    if( ax == ay ) {
        do {
            cur.y += sy;
            cur.x += sx;
            ret.push_back( cur );
        } while( ( cur.x != p2.x || cur.y != p2.y ) &&
                 ( cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax ) );
    } else if( ax > ay ) {
        do {
            if( t > 0 ) {
                cur.y += sy;
                t -= ax;
            }
            cur.x += sx;
            t += ay;
            ret.push_back( cur );
        } while( ( cur.x != p2.x || cur.y != p2.y ) &&
                 ( cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax ) );
    } else {
        do {
            if( t > 0 ) {
                cur.x += sx;
                t -= ay;
            }
            cur.y += sy;
            t += ax;
            ret.push_back( cur );
        } while( ( cur.x != p2.x || cur.y != p2.y ) &&
                 ( cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax ) );
    }
    return ret;
}

static void check_bresenham( const tripoint &source, const tripoint &destination,
                             const std::vector<tripoint> &path )
{
    std::vector<tripoint> generated_path;
    bresenham( source, destination, 0, 0, [&generated_path]( const tripoint & current_point ) {
        generated_path.push_back( current_point );
        return true;
    } );
    CAPTURE( source );
    CAPTURE( destination );
    CHECK( path == generated_path );
}

TEST_CASE( "3D_bresenham", "[line]" )
{
    check_bresenham( { 0, 0, 0 }, { -1, -1, -1 }, { { -1, -1, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, -1, 0 }, { { -1, -1, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, -1, 1 }, { { -1, -1, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, 0, -1 }, { { -1, 0, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, 0, 0 }, { { -1, 0, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, 0, 1 }, { { -1, 0, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, 1, -1 }, { { -1, 1, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, 1, 0 }, { { -1, 1, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { -1, 1, 1 }, { { -1, 1, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, -1, -1 }, { { 0, -1, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, -1, 0 }, { { 0, -1, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, -1, 1 }, { { 0, -1, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, 0, -1 }, { { 0, 0, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, 0, 0 }, { } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, 0, 1 }, { { 0, 0, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, 1, -1 }, { { 0, 1, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, 1, 0 }, { { 0, 1, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 0, 1, 1 }, { { 0, 1, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, -1, -1 }, { { 1, -1, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, -1, 0 }, { { 1, -1, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, -1, 1 }, { { 1, -1, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, 0, -1 }, { { 1, 0, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, 0, 0 }, { { 1, 0, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, 0, 1 }, { { 1, 0, 1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, 1, -1 }, { { 1, 1, -1 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, 1, 0 }, { { 1, 1, 0 } } ); // NOLINT(cata-use-named-point-constants)
    check_bresenham( { 0, 0, 0 }, { 1, 1, 1 }, { { 1, 1, 1 } } ); // NOLINT(cata-use-named-point-constants)
}

TEST_CASE( "test_normalized_angle", "[line]" )
{
    CHECK( get_normalized_angle( point_zero, {10, 0} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point_zero, {0, 10} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point_zero, {-10, 0} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point_zero, {0, -10} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point_zero, {10, 10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( point_zero, {-10, 10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( point_zero, {10, -10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( point_zero, {-10, -10} ) == Approx( 1.0 ) );
}

TEST_CASE( "Test bounds for mapping x/y/z/ offsets to direction enum", "[line]" )
{

    // Test the unit cube, which are the only values this function is valid for.
    REQUIRE( make_xyz_unit( tripoint( -1, -1, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz_unit( tripoint_north_west ) == direction::NORTHWEST );
    REQUIRE( make_xyz_unit( tripoint( -1, -1, -1 ) ) == direction::BELOWNORTHWEST );
    REQUIRE( make_xyz_unit( tripoint( 0, -1, 1 ) ) == direction::ABOVENORTH );
    REQUIRE( make_xyz_unit( tripoint_north ) == direction::NORTH );
    REQUIRE( make_xyz_unit( tripoint( 0, -1, -2 ) ) == direction::BELOWNORTH );
    REQUIRE( make_xyz_unit( tripoint( 1, -1, 1 ) ) == direction::ABOVENORTHEAST );
    REQUIRE( make_xyz_unit( tripoint_north_east ) == direction::NORTHEAST );
    REQUIRE( make_xyz_unit( tripoint( 1, -1, -1 ) ) == direction::BELOWNORTHEAST );
    REQUIRE( make_xyz_unit( tripoint( -1, 0, 1 ) ) == direction::ABOVEWEST );
    REQUIRE( make_xyz_unit( tripoint_west ) == direction::WEST );
    REQUIRE( make_xyz_unit( tripoint( -1, 0, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz_unit( tripoint_above ) == direction::ABOVECENTER );
    REQUIRE( make_xyz_unit( tripoint_zero ) == direction::CENTER );
    REQUIRE( make_xyz_unit( tripoint_below ) == direction::BELOWCENTER );
    REQUIRE( make_xyz_unit( tripoint( 1, 0, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz_unit( tripoint_east ) == direction::EAST );
    REQUIRE( make_xyz_unit( tripoint( 1, 0, -1 ) ) == direction::BELOWEAST );
    REQUIRE( make_xyz_unit( tripoint( -1, 1, 1 ) ) == direction::ABOVESOUTHWEST );
    REQUIRE( make_xyz_unit( tripoint_south_west ) == direction::SOUTHWEST );
    REQUIRE( make_xyz_unit( tripoint( -1, 1, -1 ) ) == direction::BELOWSOUTHWEST );
    REQUIRE( make_xyz_unit( tripoint( 0, 1, 1 ) ) == direction::ABOVESOUTH );
    REQUIRE( make_xyz_unit( tripoint_south ) == direction::SOUTH );
    REQUIRE( make_xyz_unit( tripoint( 0, 1, -1 ) ) == direction::BELOWSOUTH );
    REQUIRE( make_xyz_unit( tripoint( 1, 1, 1 ) ) == direction::ABOVESOUTHEAST );
    REQUIRE( make_xyz_unit( tripoint_south_east ) == direction::SOUTHEAST );
    REQUIRE( make_xyz_unit( tripoint( 1, 1, -1 ) ) == direction::BELOWSOUTHEAST );

    // Test the unit square values at distance 1 and 2.
    // Test the multiples of 30deg at 60 squares.
    // Test 22 deg to either side of the cardinal directions.
    REQUIRE( make_xyz( tripoint( -1, -1, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -2, -2, 2 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -30, -60, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -60, -60, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -60, -30, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint_north_west ) == direction::NORTHWEST );
    REQUIRE( make_xyz( tripoint( -2, -2, 0 ) ) == direction::NORTHWEST );
    REQUIRE( make_xyz( tripoint( -60, -60, 0 ) ) == direction::NORTHWEST );
    REQUIRE( make_xyz( tripoint( -1, -1, -1 ) ) == direction::BELOWNORTHWEST );
    REQUIRE( make_xyz( tripoint( -2, -2, -2 ) ) == direction::BELOWNORTHWEST );
    REQUIRE( make_xyz( tripoint( -30, -60, -1 ) ) == direction::BELOWNORTHWEST );
    REQUIRE( make_xyz( tripoint( -60, -60, -1 ) ) == direction::BELOWNORTHWEST );
    REQUIRE( make_xyz( tripoint( -60, -30, -1 ) ) == direction::BELOWNORTHWEST );
    REQUIRE( make_xyz( tripoint( 0, -1, 1 ) ) == direction::ABOVENORTH );
    REQUIRE( make_xyz( tripoint( 0, -2, 2 ) ) == direction::ABOVENORTH );
    REQUIRE( make_xyz( tripoint( -22, -60, 1 ) ) == direction::ABOVENORTH );
    REQUIRE( make_xyz( tripoint( 0, -60, 1 ) ) == direction::ABOVENORTH );
    REQUIRE( make_xyz( tripoint( 22, -60, 1 ) ) == direction::ABOVENORTH );
    REQUIRE( make_xyz( tripoint_north ) == direction::NORTH );
    REQUIRE( make_xyz( tripoint( 0, -2, 0 ) ) == direction::NORTH );
    REQUIRE( make_xyz( tripoint( -22, -60, 0 ) ) == direction::NORTH );
    REQUIRE( make_xyz( tripoint( 0, -60, 0 ) ) == direction::NORTH );
    REQUIRE( make_xyz( tripoint( 22, -60, 0 ) ) == direction::NORTH );
    REQUIRE( make_xyz( tripoint( 0, -1, -1 ) ) == direction::BELOWNORTH );
    REQUIRE( make_xyz( tripoint( 0, -2, -2 ) ) == direction::BELOWNORTH );
    REQUIRE( make_xyz( tripoint( -22, -60, -1 ) ) == direction::BELOWNORTH );
    REQUIRE( make_xyz( tripoint( 0, -60, -1 ) ) == direction::BELOWNORTH );
    REQUIRE( make_xyz( tripoint( 22, -60, -1 ) ) == direction::BELOWNORTH );
    REQUIRE( make_xyz( tripoint( 1, -1, 1 ) ) == direction::ABOVENORTHEAST );
    REQUIRE( make_xyz( tripoint( 2, -2, 2 ) ) == direction::ABOVENORTHEAST );
    REQUIRE( make_xyz( tripoint( 30, -60, 1 ) ) == direction::ABOVENORTHEAST );
    REQUIRE( make_xyz( tripoint( 60, -60, 1 ) ) == direction::ABOVENORTHEAST );
    REQUIRE( make_xyz( tripoint( 60, -30, 1 ) ) == direction::ABOVENORTHEAST );
    REQUIRE( make_xyz( tripoint_north_east ) == direction::NORTHEAST );
    REQUIRE( make_xyz( tripoint( 2, -2, 0 ) ) == direction::NORTHEAST );
    REQUIRE( make_xyz( tripoint( 30, -60, 0 ) ) == direction::NORTHEAST );
    REQUIRE( make_xyz( tripoint( 60, -60, 0 ) ) == direction::NORTHEAST );
    REQUIRE( make_xyz( tripoint( 60, -30, 0 ) ) == direction::NORTHEAST );
    REQUIRE( make_xyz( tripoint( 1, -1, -1 ) ) == direction::BELOWNORTHEAST );
    REQUIRE( make_xyz( tripoint( 2, -2, -2 ) ) == direction::BELOWNORTHEAST );
    REQUIRE( make_xyz( tripoint( 30, -60, -1 ) ) == direction::BELOWNORTHEAST );
    REQUIRE( make_xyz( tripoint( 60, -60, -1 ) ) == direction::BELOWNORTHEAST );
    REQUIRE( make_xyz( tripoint( 60, -30, -1 ) ) == direction::BELOWNORTHEAST );

    REQUIRE( make_xyz( tripoint( -1, 0, 1 ) ) == direction::ABOVEWEST );
    REQUIRE( make_xyz( tripoint( -2, 0, 2 ) ) == direction::ABOVEWEST );
    REQUIRE( make_xyz( tripoint( -60, -22, 1 ) ) == direction::ABOVEWEST );
    REQUIRE( make_xyz( tripoint( -60, 0, 1 ) ) == direction::ABOVEWEST );
    REQUIRE( make_xyz( tripoint( -60, 22, 1 ) ) == direction::ABOVEWEST );
    REQUIRE( make_xyz( tripoint_west ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -2, 0, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -60, -22, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -60, 0, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -60, 22, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -1, 0, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -2, 0, -2 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -60, -22, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -60, 0, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -60, 22, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint_above ) == direction::ABOVECENTER );
    REQUIRE( make_xyz( tripoint( 0, 0, 2 ) ) == direction::ABOVECENTER );
    REQUIRE( make_xyz( tripoint_zero ) == direction::CENTER );
    REQUIRE( make_xyz( tripoint_below ) == direction::BELOWCENTER );
    REQUIRE( make_xyz( tripoint( 0, 0, -2 ) ) == direction::BELOWCENTER );
    REQUIRE( make_xyz( tripoint( 1, 0, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 2, 0, 2 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 60, -22, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 60, 0, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 60, 22, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint_east ) == direction::EAST );
    REQUIRE( make_xyz( tripoint( 2, 0, 0 ) ) == direction::EAST );
    REQUIRE( make_xyz( tripoint( 60, -22, 0 ) ) == direction::EAST );
    REQUIRE( make_xyz( tripoint( 60, 0, 0 ) ) == direction::EAST );
    REQUIRE( make_xyz( tripoint( 60, 22, 0 ) ) == direction::EAST );
    REQUIRE( make_xyz( tripoint( 1, 0, -1 ) ) == direction::BELOWEAST );
    REQUIRE( make_xyz( tripoint( 2, 0, -2 ) ) == direction::BELOWEAST );
    REQUIRE( make_xyz( tripoint( 60, -22, -1 ) ) == direction::BELOWEAST );
    REQUIRE( make_xyz( tripoint( 60, 0, -1 ) ) == direction::BELOWEAST );
    REQUIRE( make_xyz( tripoint( 60, 22, -1 ) ) == direction::BELOWEAST );

    REQUIRE( make_xyz( tripoint( -1, 1, 1 ) ) == direction::ABOVESOUTHWEST );
    REQUIRE( make_xyz( tripoint( -2, 2, 2 ) ) == direction::ABOVESOUTHWEST );
    REQUIRE( make_xyz( tripoint( -30, 60, 1 ) ) == direction::ABOVESOUTHWEST );
    REQUIRE( make_xyz( tripoint( -60, 60, 1 ) ) == direction::ABOVESOUTHWEST );
    REQUIRE( make_xyz( tripoint( -60, 30, 1 ) ) == direction::ABOVESOUTHWEST );
    REQUIRE( make_xyz( tripoint_south_west ) == direction::SOUTHWEST );
    REQUIRE( make_xyz( tripoint( -2, 2, 0 ) ) == direction::SOUTHWEST );
    REQUIRE( make_xyz( tripoint( -30, 60, 0 ) ) == direction::SOUTHWEST );
    REQUIRE( make_xyz( tripoint( -60, 60, 0 ) ) == direction::SOUTHWEST );
    REQUIRE( make_xyz( tripoint( -60, 30, 0 ) ) == direction::SOUTHWEST );
    REQUIRE( make_xyz( tripoint( -1, 1, -1 ) ) == direction::BELOWSOUTHWEST );
    REQUIRE( make_xyz( tripoint( -2, 2, -2 ) ) == direction::BELOWSOUTHWEST );
    REQUIRE( make_xyz( tripoint( -30, 60, -1 ) ) == direction::BELOWSOUTHWEST );
    REQUIRE( make_xyz( tripoint( -60, 60, -1 ) ) == direction::BELOWSOUTHWEST );
    REQUIRE( make_xyz( tripoint( -60, 30, -1 ) ) == direction::BELOWSOUTHWEST );
    REQUIRE( make_xyz( tripoint( 0, 1, 1 ) ) == direction::ABOVESOUTH );
    REQUIRE( make_xyz( tripoint( 0, 2, 2 ) ) == direction::ABOVESOUTH );
    REQUIRE( make_xyz( tripoint( 0, 60, 1 ) ) == direction::ABOVESOUTH );
    REQUIRE( make_xyz( tripoint_south ) == direction::SOUTH );
    REQUIRE( make_xyz( tripoint( -22, 60, 0 ) ) == direction::SOUTH );
    REQUIRE( make_xyz( tripoint( 0, 60, 0 ) ) == direction::SOUTH );
    REQUIRE( make_xyz( tripoint( 22, 60, 0 ) ) == direction::SOUTH );
    REQUIRE( make_xyz( tripoint( 0, 1, -1 ) ) == direction::BELOWSOUTH );
    REQUIRE( make_xyz( tripoint( 0, 2, -2 ) ) == direction::BELOWSOUTH );
    REQUIRE( make_xyz( tripoint( -22, 60, -1 ) ) == direction::BELOWSOUTH );
    REQUIRE( make_xyz( tripoint( 0, 60, -1 ) ) == direction::BELOWSOUTH );
    REQUIRE( make_xyz( tripoint( 22, 60, -1 ) ) == direction::BELOWSOUTH );
    REQUIRE( make_xyz( tripoint( 1, 1, 1 ) ) == direction::ABOVESOUTHEAST );
    REQUIRE( make_xyz( tripoint( 2, 2, 2 ) ) == direction::ABOVESOUTHEAST );
    REQUIRE( make_xyz( tripoint( 30, 60, 1 ) ) == direction::ABOVESOUTHEAST );
    REQUIRE( make_xyz( tripoint( 60, 60, 1 ) ) == direction::ABOVESOUTHEAST );
    REQUIRE( make_xyz( tripoint( 60, 30, 1 ) ) == direction::ABOVESOUTHEAST );
    REQUIRE( make_xyz( tripoint_south_east ) == direction::SOUTHEAST );
    REQUIRE( make_xyz( tripoint( 2, 2, 0 ) ) == direction::SOUTHEAST );
    REQUIRE( make_xyz( tripoint( 30, 60, 0 ) ) == direction::SOUTHEAST );
    REQUIRE( make_xyz( tripoint( 60, 60, 0 ) ) == direction::SOUTHEAST );
    REQUIRE( make_xyz( tripoint( 60, 30, 0 ) ) == direction::SOUTHEAST );
    REQUIRE( make_xyz( tripoint( 1, 1, -1 ) ) == direction::BELOWSOUTHEAST );
    REQUIRE( make_xyz( tripoint( 2, 2, -2 ) ) == direction::BELOWSOUTHEAST );
    REQUIRE( make_xyz( tripoint( 30, 60, -1 ) ) == direction::BELOWSOUTHEAST );
    REQUIRE( make_xyz( tripoint( 60, 60, -1 ) ) == direction::BELOWSOUTHEAST );
    REQUIRE( make_xyz( tripoint( 60, 30, -1 ) ) == direction::BELOWSOUTHEAST );
}

TEST_CASE( "direction_from", "[line]" )
{
    for( int x = -2; x <= 2; ++x ) {
        for( int y = -2; y <= 2; ++y ) {
            for( int z = -2; z <= 2; ++z ) {
                tripoint p( x, y, z );
                CHECK( direction_from( tripoint_zero, p ) == direction_from( p ) );
                CHECK( direction_from( p ) == make_xyz( p ) );
            }
        }
    }
}

TEST_CASE( "direction_name", "[line]" )
{
    CHECK( direction_name( direction_from( tripoint_north_east + tripoint_above ) ) ==
           "northeast and above" );
    CHECK( direction_name_short( direction_from( tripoint_north_east + tripoint_above ) ) ==
           "UP_NE" );
}

TEST_CASE( "squares_closer_to_test", "[line]" )
{
    // TODO: make this ordering agnostic.
    auto actual = squares_closer_to( tripoint_zero, {10, 0, 0} );
    std::vector<tripoint> expected = {tripoint_east, tripoint_south_east, tripoint_north_east};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_zero, {-10, -10, 0} );
    expected = {tripoint_north_west, tripoint_west, tripoint_north};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_zero, {10, 10, 0} );
    expected = {tripoint_south_east, tripoint_east, tripoint_south};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_zero, {10, 9, 0} );
    expected = {tripoint_east, tripoint_south_east, tripoint_north_east, tripoint_south};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_zero, {10, 1, 0} );
    expected = {tripoint_east, tripoint_south_east, tripoint_north_east, tripoint_south};
    CHECK( actual == expected );

    actual = squares_closer_to( {10, 9, 0}, tripoint_zero );
    expected = {tripoint( 9, 9, 0 ), tripoint( 9, 10, 0 ), tripoint( 9, 8, 0 ), tripoint( 10, 8, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_zero, {-10, -9, 0} );
    expected = {tripoint_west, tripoint_south_west, tripoint_north_west, tripoint_north};
    CHECK( actual == expected );

    actual = squares_closer_to( {10, -10, 0}, {10, 10, 0} );
    expected = {tripoint( 10, -9, 0 ), tripoint( 11, -9, 0 ), tripoint( 9, -9, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {10, -10, 0}, {-10, -5, 0} );
    expected = {tripoint( 9, -10, 0 ), tripoint( 9, -9, 0 ), tripoint( 9, -11, 0 ), tripoint( 10, -9, 0 )};
    CHECK( actual == expected );
}

#define RANDOM_TEST_NUM 1000
#define COORDINATE_RANGE 99

static void line_to_comparison( const int iterations )
{
    REQUIRE( trig_dist( point_zero, point_zero ) == 0 );
    REQUIRE( trig_dist( point_zero, point_east ) == 1 );

    const int seed = time( nullptr );
    std::srand( seed );

    for( int i = 0; i < RANDOM_TEST_NUM; ++i ) {
        const int x1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int x2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        int t1 = 0;
        int t2 = 0;
        REQUIRE( line_to( point( x1, y1 ), point( x2, y2 ), t1 ) == canonical_line_to( point( x1, y1 ),
                 point( x2, y2 ),
                 t2 ) );
    }

    {
        const int x1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int x2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int t1 = 0;
        const int t2 = 0;
        int count1 = 0;
        const auto start1 = std::chrono::high_resolution_clock::now();
        while( count1 < iterations ) {
            line_to( point( x1, y1 ), point( x2, y2 ), t1 );
            count1++;
        }
        const auto end1 = std::chrono::high_resolution_clock::now();
        int count2 = 0;
        const auto start2 = std::chrono::high_resolution_clock::now();
        while( count2 < iterations ) {
            canonical_line_to( point( x1, y1 ), point( x2, y2 ), t2 );
            count2++;
        }
        const auto end2 = std::chrono::high_resolution_clock::now();

        if( iterations > 1 ) {
            const long long diff1 =
                std::chrono::duration_cast<std::chrono::microseconds>( end1 - start1 ).count();
            const long long diff2 =
                std::chrono::duration_cast<std::chrono::microseconds>( end2 - start2 ).count();

            printf( "line_to() executed %d times in %lld microseconds.\n",
                    iterations, diff1 );
            printf( "canonical_line_to() executed %d times in %lld microseconds.\n",
                    iterations, diff2 );
        }
    }
}

// Check the boundaries of inputs we can give line_to without breaking it.
TEST_CASE( "line_to_boundaries", "[line]" )
{
    for( int i = -60; i < 60; ++i ) {
        for( int j = -60; j < 60; ++j ) {
            const int ax = std::abs( i ) * 2;
            const int ay = std::abs( j ) * 2;
            const int dominant = std::max( ax, ay );
            const int minor = std::min( ax, ay );
            const int ideal_start_offset = minor - ( dominant / 2 );
            // get the sign of the start offset.
            const int st( ( ideal_start_offset > 0 ) - ( ideal_start_offset < 0 ) );
            const int max_start_offset = std::abs( ideal_start_offset ) * 2 + 1;
            for( int k = -1; k <= max_start_offset; ++k ) {
                auto line = line_to( point_zero, point( i, j ), k * st );
                if( line.back() != point( i, j ) ) {
                    WARN( "Expected (" << i << "," << j << ") but got (" <<
                          line.back().x << "," << line.back().y << ") with t == " << k );
                }
                CHECK( line.back() == point( i, j ) );
            }
        }
    }
}

TEST_CASE( "line_to_regression", "[line]" )
{
    line_to_comparison( 1 );
}

TEST_CASE( "line_to_performance", "[.]" )
{
    line_to_comparison( 10000 );
}
