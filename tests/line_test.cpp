#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "cata_generators.h"
#include "coordinates.h"
#include "line.h"
#include "point.h"
#include "rng.h"

#define SGN(a) (((a)<0) ? -1 : 1)
// Compare all future line_to implementations to the canonical one.
static std::vector <point> canonical_line_to( const point &p1, const point &p2, int t )
{
    std::vector<point> ret;
    const point d( -p1 + p2 );
    const point a( std::abs( d.x ) << 1, std::abs( d.y ) << 1 );
    point s( SGN( d.x ), SGN( d.y ) );
    if( d.y == 0 ) {
        s.y = 0;
    }
    if( d.x == 0 ) {
        s.x = 0;
    }
    point cur;
    cur.x = p1.x;
    cur.y = p1.y;

    point min( std::min( p1.x, p2.x ), std::min( p1.y, p2.y ) );
    int xmax = std::max( p1.x, p2.x );
    int ymax = std::max( p1.y, p2.y );

    min.x -= std::abs( d.x );
    min.y -= std::abs( d.y );
    xmax += std::abs( d.x );
    ymax += std::abs( d.y );

    if( a.x == a.y ) {
        do {
            cur.y += s.y;
            cur.x += s.x;
            ret.push_back( cur );
        } while( ( cur.x != p2.x || cur.y != p2.y ) &&
                 ( cur.x >= min.x && cur.x <= xmax && cur.y >= min.y && cur.y <= ymax ) );
    } else if( a.x > a.y ) {
        do {
            if( t > 0 ) {
                cur.y += s.y;
                t -= a.x;
            }
            cur.x += s.x;
            t += a.y;
            ret.push_back( cur );
        } while( ( cur.x != p2.x || cur.y != p2.y ) &&
                 ( cur.x >= min.x && cur.x <= xmax && cur.y >= min.y && cur.y <= ymax ) );
    } else {
        do {
            if( t > 0 ) {
                cur.x += s.x;
                t -= a.y;
            }
            cur.y += s.y;
            t += a.x;
            ret.push_back( cur );
        } while( ( cur.x != p2.x || cur.y != p2.y ) &&
                 ( cur.x >= min.x && cur.x <= xmax && cur.y >= min.y && cur.y <= ymax ) );
    }
    return ret;
}

static void check_bresenham( const tripoint_bub_ms &source, const tripoint_bub_ms &destination,
                             const std::vector<tripoint_bub_ms> &path )
{
    std::vector<tripoint_bub_ms> generated_path;
    bresenham( source, destination, 0, 0, [&generated_path]( const tripoint_bub_ms & current_point ) {
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
    CHECK( get_normalized_angle( point::zero, {10, 0} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point::zero, {0, 10} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point::zero, {-10, 0} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point::zero, {0, -10} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( point::zero, {10, 10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( point::zero, {-10, 10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( point::zero, {10, -10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( point::zero, {-10, -10} ) == Approx( 1.0 ) );
}

// NOLINTNEXTLINE(readability-function-size)
TEST_CASE( "Test_bounds_for_mapping_x/y/z/_offsets_to_direction_enum", "[line]" )
{
    // Test the unit cube, which are the only values this function is valid for.
    REQUIRE( make_xyz_unit( tripoint( -1, -1, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz_unit( tripoint::north_west ) == direction::NORTHWEST );
    REQUIRE( make_xyz_unit( tripoint( -1, -1, -1 ) ) == direction::BELOWNORTHWEST );
    REQUIRE( make_xyz_unit( tripoint( 0, -1, 1 ) ) == direction::ABOVENORTH );
    REQUIRE( make_xyz_unit( tripoint::north ) == direction::NORTH );
    REQUIRE( make_xyz_unit( tripoint( 0, -1, -2 ) ) == direction::BELOWNORTH );
    REQUIRE( make_xyz_unit( tripoint( 1, -1, 1 ) ) == direction::ABOVENORTHEAST );
    REQUIRE( make_xyz_unit( tripoint::north_east ) == direction::NORTHEAST );
    REQUIRE( make_xyz_unit( tripoint( 1, -1, -1 ) ) == direction::BELOWNORTHEAST );
    REQUIRE( make_xyz_unit( tripoint( -1, 0, 1 ) ) == direction::ABOVEWEST );
    REQUIRE( make_xyz_unit( tripoint::west ) == direction::WEST );
    REQUIRE( make_xyz_unit( tripoint( -1, 0, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz_unit( tripoint::above ) == direction::ABOVECENTER );
    REQUIRE( make_xyz_unit( tripoint::zero ) == direction::CENTER );
    REQUIRE( make_xyz_unit( tripoint::below ) == direction::BELOWCENTER );
    REQUIRE( make_xyz_unit( tripoint( 1, 0, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz_unit( tripoint::east ) == direction::EAST );
    REQUIRE( make_xyz_unit( tripoint( 1, 0, -1 ) ) == direction::BELOWEAST );
    REQUIRE( make_xyz_unit( tripoint( -1, 1, 1 ) ) == direction::ABOVESOUTHWEST );
    REQUIRE( make_xyz_unit( tripoint::south_west ) == direction::SOUTHWEST );
    REQUIRE( make_xyz_unit( tripoint( -1, 1, -1 ) ) == direction::BELOWSOUTHWEST );
    REQUIRE( make_xyz_unit( tripoint( 0, 1, 1 ) ) == direction::ABOVESOUTH );
    REQUIRE( make_xyz_unit( tripoint::south ) == direction::SOUTH );
    REQUIRE( make_xyz_unit( tripoint( 0, 1, -1 ) ) == direction::BELOWSOUTH );
    REQUIRE( make_xyz_unit( tripoint( 1, 1, 1 ) ) == direction::ABOVESOUTHEAST );
    REQUIRE( make_xyz_unit( tripoint::south_east ) == direction::SOUTHEAST );
    REQUIRE( make_xyz_unit( tripoint( 1, 1, -1 ) ) == direction::BELOWSOUTHEAST );

    // Test the unit square values at distance 1 and 2.
    // Test the multiples of 30deg at 60 squares.
    // Test 22 deg to either side of the cardinal directions.
    REQUIRE( make_xyz( tripoint( -1, -1, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -2, -2, 2 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -30, -60, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -60, -60, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint( -60, -30, 1 ) ) == direction::ABOVENORTHWEST );
    REQUIRE( make_xyz( tripoint::north_west ) == direction::NORTHWEST );
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
    REQUIRE( make_xyz( tripoint::north ) == direction::NORTH );
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
    REQUIRE( make_xyz( tripoint::north_east ) == direction::NORTHEAST );
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
    REQUIRE( make_xyz( tripoint::west ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -2, 0, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -60, -22, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -60, 0, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -60, 22, 0 ) ) == direction::WEST );
    REQUIRE( make_xyz( tripoint( -1, 0, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -2, 0, -2 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -60, -22, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -60, 0, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint( -60, 22, -1 ) ) == direction::BELOWWEST );
    REQUIRE( make_xyz( tripoint::above ) == direction::ABOVECENTER );
    REQUIRE( make_xyz( tripoint( 0, 0, 2 ) ) == direction::ABOVECENTER );
    REQUIRE( make_xyz( tripoint::zero ) == direction::CENTER );
    REQUIRE( make_xyz( tripoint::below ) == direction::BELOWCENTER );
    REQUIRE( make_xyz( tripoint( 0, 0, -2 ) ) == direction::BELOWCENTER );
    REQUIRE( make_xyz( tripoint( 1, 0, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 2, 0, 2 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 60, -22, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 60, 0, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint( 60, 22, 1 ) ) == direction::ABOVEEAST );
    REQUIRE( make_xyz( tripoint::east ) == direction::EAST );
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
    REQUIRE( make_xyz( tripoint::south_west ) == direction::SOUTHWEST );
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
    REQUIRE( make_xyz( tripoint::south ) == direction::SOUTH );
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
    REQUIRE( make_xyz( tripoint::south_east ) == direction::SOUTHEAST );
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

TEST_CASE( "direction_from", "[point][line][coords]" )
{
    for( int x = -2; x <= 2; ++x ) {
        for( int y = -2; y <= 2; ++y ) {
            for( int z = -2; z <= 2; ++z ) {
                tripoint p( x, y, z );
                tripoint_abs_omt c( p );
                CHECK( direction_from( tripoint::zero, p ) == direction_from( p ) );
                CHECK( direction_from( tripoint::zero, p ) ==
                       direction_from( tripoint_abs_omt(), c ) );
                CHECK( direction_from( p ) == make_xyz( p ) );
            }
        }
    }
}

TEST_CASE( "direction_name", "[line]" )
{
    CHECK( direction_name( direction_from( tripoint::north_east + tripoint::above ) ) ==
           "northeast and above" );
    CHECK( direction_name_short( direction_from( tripoint::north_east + tripoint::above ) ) ==
           "UP_NE" );
}

TEST_CASE( "squares_closer_to_test", "[line]" )
{
    // TODO: make this ordering agnostic.
    auto actual = squares_closer_to( tripoint_bub_ms::zero, {10, 0, 0} );
    std::vector<tripoint_bub_ms> expected = {tripoint_bub_ms::zero + tripoint::east, tripoint_bub_ms::zero + tripoint::south_east, tripoint_bub_ms::zero + tripoint::north_east};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_bub_ms::zero, {-10, -10, 0} );
    expected = { tripoint_bub_ms::zero + tripoint::north_west, tripoint_bub_ms::zero + tripoint::west, tripoint_bub_ms::zero + tripoint::north};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_bub_ms::zero, {10, 10, 0} );
    expected = { tripoint_bub_ms::zero + tripoint::south_east, tripoint_bub_ms::zero + tripoint::east, tripoint_bub_ms::zero + tripoint::south};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_bub_ms::zero, {10, 9, 0} );
    expected = { tripoint_bub_ms::zero + tripoint::east, tripoint_bub_ms::zero + tripoint::south_east, tripoint_bub_ms::zero + tripoint::north_east, tripoint_bub_ms::zero + tripoint::south};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_bub_ms::zero, {10, 1, 0} );
    expected = { tripoint_bub_ms::zero + tripoint::east, tripoint_bub_ms::zero + tripoint::south_east, tripoint_bub_ms::zero + tripoint::north_east, tripoint_bub_ms::zero + tripoint::south};
    CHECK( actual == expected );

    actual = squares_closer_to( {10, 9, 0}, tripoint_bub_ms::zero );
    expected = { tripoint_bub_ms( 9, 9, 0 ), tripoint_bub_ms( 9, 10, 0 ), tripoint_bub_ms( 9, 8, 0 ), tripoint_bub_ms( 10, 8, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( tripoint_bub_ms::zero, {-10, -9, 0} );
    expected = { tripoint_bub_ms::zero + tripoint::west, tripoint_bub_ms::zero + tripoint::south_west, tripoint_bub_ms::zero + tripoint::north_west, tripoint_bub_ms::zero + tripoint::north};
    CHECK( actual == expected );

    actual = squares_closer_to( {10, -10, 0}, {10, 10, 0} );
    expected = {tripoint_bub_ms( 10, -9, 0 ), tripoint_bub_ms( 11, -9, 0 ), tripoint_bub_ms( 9, -9, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {10, -10, 0}, {-10, -5, 0} );
    expected = {tripoint_bub_ms( 9, -10, 0 ), tripoint_bub_ms( 9, -9, 0 ), tripoint_bub_ms( 9, -11, 0 ), tripoint_bub_ms( 10, -9, 0 )};
    CHECK( actual == expected );
}

static constexpr int RANDOM_TEST_NUM = 1000;
static constexpr int COORDINATE_RANGE = 99;

static void line_to_comparison( const int iterations )
{
    REQUIRE( trig_dist( point::zero, point::zero ) == 0 );
    REQUIRE( trig_dist( point::zero, point::east ) == 1 );

    for( int i = 0; i < RANDOM_TEST_NUM; ++i ) {
        const point p1( rng( -COORDINATE_RANGE, COORDINATE_RANGE ), rng( -COORDINATE_RANGE,
                        COORDINATE_RANGE ) );
        const point p2( rng( -COORDINATE_RANGE, COORDINATE_RANGE ), rng( -COORDINATE_RANGE,
                        COORDINATE_RANGE ) );
        int t1 = 0;
        int t2 = 0;
        REQUIRE( line_to( p1, p2, t1 ) == canonical_line_to( p1,
                 p2,
                 t2 ) );
    }

    {
        const point p12( rng( -COORDINATE_RANGE, COORDINATE_RANGE ), rng( -COORDINATE_RANGE,
                         COORDINATE_RANGE ) );
        const point p22( rng( -COORDINATE_RANGE, COORDINATE_RANGE ), rng( -COORDINATE_RANGE,
                         COORDINATE_RANGE ) );
        const int t1 = 0;
        const int t2 = 0;
        int count1 = 0;
        const std::chrono::high_resolution_clock::time_point start1 =
            std::chrono::high_resolution_clock::now();
        while( count1 < iterations ) {
            line_to( p12, p22, t1 );
            count1++;
        }
        const std::chrono::high_resolution_clock::time_point end1 =
            std::chrono::high_resolution_clock::now();
        int count2 = 0;
        const std::chrono::high_resolution_clock::time_point start2 =
            std::chrono::high_resolution_clock::now();
        while( count2 < iterations ) {
            canonical_line_to( p12, p22, t2 );
            count2++;
        }
        const std::chrono::high_resolution_clock::time_point end2 =
            std::chrono::high_resolution_clock::now();

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
            const point a( std::abs( i ) * 2, std::abs( j ) * 2 );
            const int dominant = std::max( a.x, a.y );
            const int minor = std::min( a.x, a.y );
            const int ideal_start_offset = minor - ( dominant / 2 );
            // get the sign of the start offset.
            const int st( ( ideal_start_offset > 0 ) - ( ideal_start_offset < 0 ) );
            const int max_start_offset = std::abs( ideal_start_offset ) * 2 + 1;
            for( int k = -1; k <= max_start_offset; ++k ) {
                auto line = line_to( point::zero, point( i, j ), k * st );
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

TEST_CASE( "coord_point_line_to_consistency", "[point][coords][line]" )
{
    point p0 = GENERATE( take( 5, random_points() ) );
    point p1 = GENERATE( take( 5, random_points() ) );
    CAPTURE( p0, p1 );
    point_abs_ms cp0( p0 );
    point_abs_ms cp1( p1 );

    std::vector<point> raw_line = line_to( p0, p1 );
    std::vector<point_abs_ms> coord_line = line_to( cp0, cp1 );

    REQUIRE( raw_line.size() == coord_line.size() );
    for( size_t i = 0; i < raw_line.size(); ++i ) {
        CAPTURE( i );
        CHECK( raw_line[i] == coord_line[i].raw() );
    }
}
