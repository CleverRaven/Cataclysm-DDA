#include <chrono>
#include <cstdio>

#include "catch/catch.hpp"
#include "line.h"
#include "rng.h"

#define SGN(a) (((a)<0) ? -1 : 1)
// Compare all future line_to implementations to the canonical one.
std::vector <point> canonical_line_to( const int x1, const int y1, const int x2, const int y2,
                                       int t )
{
    std::vector<point> ret;
    const int dx = x2 - x1;
    const int dy = y2 - y1;
    const int ax = abs( dx ) << 1;
    const int ay = abs( dy ) << 1;
    int sx = SGN( dx );
    int sy = SGN( dy );
    if( dy == 0 ) {
        sy = 0;
    }
    if( dx == 0 ) {
        sx = 0;
    }
    point cur;
    cur.x = x1;
    cur.y = y1;

    int xmin = ( x1 < x2 ? x1 : x2 );
    int ymin = ( y1 < y2 ? y1 : y2 );
    int xmax = ( x1 > x2 ? x1 : x2 );
    int ymax = ( y1 > y2 ? y1 : y2 );

    xmin -= abs( dx );
    ymin -= abs( dy );
    xmax += abs( dx );
    ymax += abs( dy );

    if( ax == ay ) {
        do {
            cur.y += sy;
            cur.x += sx;
            ret.push_back( cur );
        } while( ( cur.x != x2 || cur.y != y2 ) &&
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
        } while( ( cur.x != x2 || cur.y != y2 ) &&
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
        } while( ( cur.x != x2 || cur.y != y2 ) &&
                 ( cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax ) );
    }
    return ret;
}

TEST_CASE( "test_normalized_angle" )
{
    CHECK( get_normalized_angle( {0, 0}, {10, 0} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( {0, 0}, {0, 10} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( {0, 0}, {-10, 0} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( {0, 0}, {0, -10} ) == Approx( 0.0 ) );
    CHECK( get_normalized_angle( {0, 0}, {10, 10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( {0, 0}, {-10, 10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( {0, 0}, {10, -10} ) == Approx( 1.0 ) );
    CHECK( get_normalized_angle( {0, 0}, {-10, -10} ) == Approx( 1.0 ) );
}

TEST_CASE( "Test bounds for mapping x/y/z/ offsets to direction enum" )
{

    // Test the unit cube, which are the only values this function is valid for.
    REQUIRE( make_xyz_unit( -1, -1, -1 ) == ABOVENORTHWEST );
    REQUIRE( make_xyz_unit( -1, -1, 0 ) == NORTHWEST );
    REQUIRE( make_xyz_unit( -1, -1, 1 ) == BELOWNORTHWEST );
    REQUIRE( make_xyz_unit( 0, -1, -1 ) == ABOVENORTH );
    REQUIRE( make_xyz_unit( 0, -1, 0 ) == NORTH );
    REQUIRE( make_xyz_unit( 0, -1, 2 ) == BELOWNORTH );
    REQUIRE( make_xyz_unit( 1, -1, -1 ) == ABOVENORTHEAST );
    REQUIRE( make_xyz_unit( 1, -1, 0 ) == NORTHEAST );
    REQUIRE( make_xyz_unit( 1, -1, 1 ) == BELOWNORTHEAST );
    REQUIRE( make_xyz_unit( -1, 0, -1 ) == ABOVEWEST );
    REQUIRE( make_xyz_unit( -1, 0, 0 ) == WEST );
    REQUIRE( make_xyz_unit( -1, 0, 1 ) == BELOWWEST );
    REQUIRE( make_xyz_unit( 0, 0, -1 ) == ABOVECENTER );
    REQUIRE( make_xyz_unit( 0, 0, 0 ) == CENTER );
    REQUIRE( make_xyz_unit( 0, 0, 1 ) == BELOWCENTER );
    REQUIRE( make_xyz_unit( 1, 0, -1 ) == ABOVEEAST );
    REQUIRE( make_xyz_unit( 1, 0, 0 ) == EAST );
    REQUIRE( make_xyz_unit( 1, 0, 1 ) == BELOWEAST );
    REQUIRE( make_xyz_unit( -1, 1, -1 ) == ABOVESOUTHWEST );
    REQUIRE( make_xyz_unit( -1, 1, 0 ) == SOUTHWEST );
    REQUIRE( make_xyz_unit( -1, 1, 1 ) == BELOWSOUTHWEST );
    REQUIRE( make_xyz_unit( 0, 1, -1 ) == ABOVESOUTH );
    REQUIRE( make_xyz_unit( 0, 1, 0 ) == SOUTH );
    REQUIRE( make_xyz_unit( 0, 1, 1 ) == BELOWSOUTH );
    REQUIRE( make_xyz_unit( 1, 1, -1 ) == ABOVESOUTHEAST );
    REQUIRE( make_xyz_unit( 1, 1, 0 ) == SOUTHEAST );
    REQUIRE( make_xyz_unit( 1, 1, 1 ) == BELOWSOUTHEAST );

    // Test the unit square values at distance 1 and 2.
    // Test the multiples of 30deg at 60 squares.
    // Test 22 deg to either side of the cardinal directions.
    REQUIRE( make_xyz( -1, -1, -1 ) == ABOVENORTHWEST );
    REQUIRE( make_xyz( -2, -2, -2 ) == ABOVENORTHWEST );
    REQUIRE( make_xyz( -30, -60, -1 ) == ABOVENORTHWEST );
    REQUIRE( make_xyz( -60, -60, -1 ) == ABOVENORTHWEST );
    REQUIRE( make_xyz( -60, -30, -1 ) == ABOVENORTHWEST );
    REQUIRE( make_xyz( -1, -1, 0 ) == NORTHWEST );
    REQUIRE( make_xyz( -2, -2, 0 ) == NORTHWEST );
    REQUIRE( make_xyz( -60, -60, 0 ) == NORTHWEST );
    REQUIRE( make_xyz( -1, -1, 1 ) == BELOWNORTHWEST );
    REQUIRE( make_xyz( -2, -2, 2 ) == BELOWNORTHWEST );
    REQUIRE( make_xyz( -30, -60, 1 ) == BELOWNORTHWEST );
    REQUIRE( make_xyz( -60, -60, 1 ) == BELOWNORTHWEST );
    REQUIRE( make_xyz( -60, -30, 1 ) == BELOWNORTHWEST );
    REQUIRE( make_xyz( 0, -1, -1 ) == ABOVENORTH );
    REQUIRE( make_xyz( 0, -2, -2 ) == ABOVENORTH );
    REQUIRE( make_xyz( -22, -60, -1 ) == ABOVENORTH );
    REQUIRE( make_xyz( 0, -60, -1 ) == ABOVENORTH );
    REQUIRE( make_xyz( 22, -60, -1 ) == ABOVENORTH );
    REQUIRE( make_xyz( 0, -1, 0 ) == NORTH );
    REQUIRE( make_xyz( 0, -2, 0 ) == NORTH );
    REQUIRE( make_xyz( -22, -60, 0 ) == NORTH );
    REQUIRE( make_xyz( 0, -60, 0 ) == NORTH );
    REQUIRE( make_xyz( 22, -60, 0 ) == NORTH );
    REQUIRE( make_xyz( 0, -1, 1 ) == BELOWNORTH );
    REQUIRE( make_xyz( 0, -2, 2 ) == BELOWNORTH );
    REQUIRE( make_xyz( -22, -60, 1 ) == BELOWNORTH );
    REQUIRE( make_xyz( 0, -60, 1 ) == BELOWNORTH );
    REQUIRE( make_xyz( 22, -60, 1 ) == BELOWNORTH );
    REQUIRE( make_xyz( 1, -1, -1 ) == ABOVENORTHEAST );
    REQUIRE( make_xyz( 2, -2, -2 ) == ABOVENORTHEAST );
    REQUIRE( make_xyz( 30, -60, -1 ) == ABOVENORTHEAST );
    REQUIRE( make_xyz( 60, -60, -1 ) == ABOVENORTHEAST );
    REQUIRE( make_xyz( 60, -30, -1 ) == ABOVENORTHEAST );
    REQUIRE( make_xyz( 1, -1, 0 ) == NORTHEAST );
    REQUIRE( make_xyz( 2, -2, 0 ) == NORTHEAST );
    REQUIRE( make_xyz( 30, -60, 0 ) == NORTHEAST );
    REQUIRE( make_xyz( 60, -60, 0 ) == NORTHEAST );
    REQUIRE( make_xyz( 60, -30, 0 ) == NORTHEAST );
    REQUIRE( make_xyz( 1, -1, 1 ) == BELOWNORTHEAST );
    REQUIRE( make_xyz( 2, -2, 2 ) == BELOWNORTHEAST );
    REQUIRE( make_xyz( 30, -60, 1 ) == BELOWNORTHEAST );
    REQUIRE( make_xyz( 60, -60, 1 ) == BELOWNORTHEAST );
    REQUIRE( make_xyz( 60, -30, 1 ) == BELOWNORTHEAST );

    REQUIRE( make_xyz( -1, 0, -1 ) == ABOVEWEST );
    REQUIRE( make_xyz( -2, 0, -2 ) == ABOVEWEST );
    REQUIRE( make_xyz( -60, -22, -1 ) == ABOVEWEST );
    REQUIRE( make_xyz( -60, 0, -1 ) == ABOVEWEST );
    REQUIRE( make_xyz( -60, 22, -1 ) == ABOVEWEST );
    REQUIRE( make_xyz( -1, 0, 0 ) == WEST );
    REQUIRE( make_xyz( -2, 0, 0 ) == WEST );
    REQUIRE( make_xyz( -60, -22, 0 ) == WEST );
    REQUIRE( make_xyz( -60, 0, 0 ) == WEST );
    REQUIRE( make_xyz( -60, 22, 0 ) == WEST );
    REQUIRE( make_xyz( -1, 0, 1 ) == BELOWWEST );
    REQUIRE( make_xyz( -2, 0, 2 ) == BELOWWEST );
    REQUIRE( make_xyz( -60, -22, 1 ) == BELOWWEST );
    REQUIRE( make_xyz( -60, 0, 1 ) == BELOWWEST );
    REQUIRE( make_xyz( -60, 22, 1 ) == BELOWWEST );
    REQUIRE( make_xyz( 0, 0, -1 ) == ABOVECENTER );
    REQUIRE( make_xyz( 0, 0, -2 ) == ABOVECENTER );
    REQUIRE( make_xyz( 0, 0, 0 ) == CENTER );
    REQUIRE( make_xyz( 0, 0, 1 ) == BELOWCENTER );
    REQUIRE( make_xyz( 0, 0, 2 ) == BELOWCENTER );
    REQUIRE( make_xyz( 1, 0, -1 ) == ABOVEEAST );
    REQUIRE( make_xyz( 2, 0, -2 ) == ABOVEEAST );
    REQUIRE( make_xyz( 60, -22, -1 ) == ABOVEEAST );
    REQUIRE( make_xyz( 60, 0, -1 ) == ABOVEEAST );
    REQUIRE( make_xyz( 60, 22, -1 ) == ABOVEEAST );
    REQUIRE( make_xyz( 1, 0, 0 ) == EAST );
    REQUIRE( make_xyz( 2, 0, 0 ) == EAST );
    REQUIRE( make_xyz( 60, -22, 0 ) == EAST );
    REQUIRE( make_xyz( 60, 0, 0 ) == EAST );
    REQUIRE( make_xyz( 60, 22, 0 ) == EAST );
    REQUIRE( make_xyz( 1, 0, 1 ) == BELOWEAST );
    REQUIRE( make_xyz( 2, 0, 2 ) == BELOWEAST );
    REQUIRE( make_xyz( 60, -22, 1 ) == BELOWEAST );
    REQUIRE( make_xyz( 60, 0, 1 ) == BELOWEAST );
    REQUIRE( make_xyz( 60, 22, 1 ) == BELOWEAST );

    REQUIRE( make_xyz( -1, 1, -1 ) == ABOVESOUTHWEST );
    REQUIRE( make_xyz( -2, 2, -2 ) == ABOVESOUTHWEST );
    REQUIRE( make_xyz( -30, 60, -1 ) == ABOVESOUTHWEST );
    REQUIRE( make_xyz( -60, 60, -1 ) == ABOVESOUTHWEST );
    REQUIRE( make_xyz( -60, 30, -1 ) == ABOVESOUTHWEST );
    REQUIRE( make_xyz( -1, 1, 0 ) == SOUTHWEST );
    REQUIRE( make_xyz( -2, 2, 0 ) == SOUTHWEST );
    REQUIRE( make_xyz( -30, 60, 0 ) == SOUTHWEST );
    REQUIRE( make_xyz( -60, 60, 0 ) == SOUTHWEST );
    REQUIRE( make_xyz( -60, 30, 0 ) == SOUTHWEST );
    REQUIRE( make_xyz( -1, 1, 1 ) == BELOWSOUTHWEST );
    REQUIRE( make_xyz( -2, 2, 2 ) == BELOWSOUTHWEST );
    REQUIRE( make_xyz( -30, 60, 1 ) == BELOWSOUTHWEST );
    REQUIRE( make_xyz( -60, 60, 1 ) == BELOWSOUTHWEST );
    REQUIRE( make_xyz( -60, 30, 1 ) == BELOWSOUTHWEST );
    REQUIRE( make_xyz( 0, 1, -1 ) == ABOVESOUTH );
    REQUIRE( make_xyz( 0, 2, -2 ) == ABOVESOUTH );
    REQUIRE( make_xyz( 0, 60, -1 ) == ABOVESOUTH );
    REQUIRE( make_xyz( 0, 1, 0 ) == SOUTH );
    REQUIRE( make_xyz( -22, 60, 0 ) == SOUTH );
    REQUIRE( make_xyz( 0, 60, 0 ) == SOUTH );
    REQUIRE( make_xyz( 22, 60, 0 ) == SOUTH );
    REQUIRE( make_xyz( 0, 1, 1 ) == BELOWSOUTH );
    REQUIRE( make_xyz( 0, 2, 2 ) == BELOWSOUTH );
    REQUIRE( make_xyz( -22, 60, 1 ) == BELOWSOUTH );
    REQUIRE( make_xyz( 0, 60, 1 ) == BELOWSOUTH );
    REQUIRE( make_xyz( 22, 60, 1 ) == BELOWSOUTH );
    REQUIRE( make_xyz( 1, 1, -1 ) == ABOVESOUTHEAST );
    REQUIRE( make_xyz( 2, 2, -2 ) == ABOVESOUTHEAST );
    REQUIRE( make_xyz( 30, 60, -1 ) == ABOVESOUTHEAST );
    REQUIRE( make_xyz( 60, 60, -1 ) == ABOVESOUTHEAST );
    REQUIRE( make_xyz( 60, 30, -1 ) == ABOVESOUTHEAST );
    REQUIRE( make_xyz( 1, 1, 0 ) == SOUTHEAST );
    REQUIRE( make_xyz( 2, 2, 0 ) == SOUTHEAST );
    REQUIRE( make_xyz( 30, 60, 0 ) == SOUTHEAST );
    REQUIRE( make_xyz( 60, 60, 0 ) == SOUTHEAST );
    REQUIRE( make_xyz( 60, 30, 0 ) == SOUTHEAST );
    REQUIRE( make_xyz( 1, 1, 1 ) == BELOWSOUTHEAST );
    REQUIRE( make_xyz( 2, 2, 2 ) == BELOWSOUTHEAST );
    REQUIRE( make_xyz( 30, 60, 1 ) == BELOWSOUTHEAST );
    REQUIRE( make_xyz( 60, 60, 1 ) == BELOWSOUTHEAST );
    REQUIRE( make_xyz( 60, 30, 1 ) == BELOWSOUTHEAST );
}

TEST_CASE( "squares_closer_to_test" )
{
    // TODO: make this ordering agnostic.
    auto actual = squares_closer_to( {0, 0, 0}, {10, 0, 0} );
    std::vector<tripoint> expected = {tripoint( 1, 0, 0 ), tripoint( 1, 1, 0 ), tripoint( 1, -1, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {0, 0, 0}, {-10, -10, 0} );
    expected = {tripoint( -1, -1, 0 ), tripoint( -1, 0, 0 ), tripoint( 0, -1, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {0, 0, 0}, {10, 10, 0} );
    expected = {tripoint( 1, 1, 0 ), tripoint( 1, 0, 0 ), tripoint( 0, 1, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {0, 0, 0}, {10, 9, 0} );
    expected = {tripoint( 1, 0, 0 ), tripoint( 1, 1, 0 ), tripoint( 1, -1, 0 ), tripoint( 0, 1, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {0, 0, 0}, {10, 1, 0} );
    expected = {tripoint( 1, 0, 0 ), tripoint( 1, 1, 0 ), tripoint( 1, -1, 0 ), tripoint( 0, 1, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {10, 9, 0}, {0, 0, 0} );
    expected = {tripoint( 9, 9, 0 ), tripoint( 9, 10, 0 ), tripoint( 9, 8, 0 ), tripoint( 10, 8, 0 )};
    CHECK( actual == expected );

    actual = squares_closer_to( {0, 0, 0}, {-10, -9, 0} );
    expected = {tripoint( -1, 0, 0 ), tripoint( -1, 1, 0 ), tripoint( -1, -1, 0 ), tripoint( 0, -1, 0 )};
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

void line_to_comparison( const int iterations )
{
    REQUIRE( trig_dist( 0, 0, 0, 0 ) == 0 );
    REQUIRE( trig_dist( 0, 0, 1, 0 ) == 1 );

    const int seed = time( NULL );
    std::srand( seed );

    for( int i = 0; i < RANDOM_TEST_NUM; ++i ) {
        const int x1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int x2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        int t1 = 0;
        int t2 = 0;
        REQUIRE( line_to( x1, y1, x2, y2, t1 ) == canonical_line_to( x1, y1, x2, y2, t2 ) );
    }

    {
        const int x1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y1 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int x2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int y2 = rng( -COORDINATE_RANGE, COORDINATE_RANGE );
        const int t1 = 0;
        const int t2 = 0;
        long count1 = 0;
        const auto start1 = std::chrono::high_resolution_clock::now();
        while( count1 < iterations ) {
            line_to( x1, y1, x2, y2, t1 );
            count1++;
        }
        const auto end1 = std::chrono::high_resolution_clock::now();
        long count2 = 0;
        const auto start2 = std::chrono::high_resolution_clock::now();
        while( count2 < iterations ) {
            canonical_line_to( x1, y1, x2, y2, t2 );
            count2++;
        }
        const auto end2 = std::chrono::high_resolution_clock::now();

        if( iterations > 1 ) {
            const long diff1 = std::chrono::duration_cast<std::chrono::microseconds>( end1 - start1 ).count();
            const long diff2 = std::chrono::duration_cast<std::chrono::microseconds>( end2 - start2 ).count();

            printf( "line_to() executed %d times in %ld microseconds.\n",
                    iterations, diff1 );
            printf( "canonical_line_to() executed %d times in %ld microseconds.\n",
                    iterations, diff2 );
        }
    }
}

// Check the boundaries of inputs we can give line_to without breaking it.
TEST_CASE( "line_to_boundaries" )
{
    for( int i = -60; i < 60; ++i ) {
        for( int j = -60; j < 60; ++j ) {
            const int ax = abs( i ) * 2;
            const int ay = abs( j ) * 2;
            const int dominant = std::max( ax, ay );
            const int minor = std::min( ax, ay );
            const int ideal_start_offset = minor - ( dominant / 2 );
            // get the sign of the start offset.
            const int st( ( ideal_start_offset > 0 ) - ( ideal_start_offset < 0 ) );
            const int max_start_offset = std::abs( ideal_start_offset ) * 2 + 1;
            for( int k = -1; k <= max_start_offset; ++k ) {
                auto line = line_to( 0, 0, i, j, k * st );
                if( line.back() != point( i, j ) ) {
                    WARN( "Expected (" << i << "," << j << ") but got (" <<
                          line.back().x << "," << line.back().y << ") with t == " << k );
                }
                CHECK( line.back() == point( i, j ) );
            }
        }
    }
}

TEST_CASE( "line_to_regression" )
{
    line_to_comparison( 1 );
}

TEST_CASE( "line_to_performance", "[.]" )
{
    line_to_comparison( 10000 );
}
