#include <string>
#include <vector>

#include "catch/catch.hpp"
#include "coordinates.h"
#include "point.h"

TEST_CASE( "rectangle_containment", "[point]" )
{
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    half_open_rectangle r1( point( 0, 0 ), point( 2, 2 ) );
    CHECK( !r1.contains( point( 0, -1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r1.contains( point( 0, 0 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r1.contains( point( 0, 1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( !r1.contains( point( 0, 2 ) ) );
    CHECK( !r1.contains( point( 0, 3 ) ) );

    // NOLINTNEXTLINE(cata-use-named-point-constants)
    inclusive_rectangle r2( point( 0, 0 ), point( 2, 2 ) );
    CHECK( !r2.contains( point( 0, -1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r2.contains( point( 0, 0 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r2.contains( point( 0, 1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r2.contains( point( 0, 2 ) ) );
    CHECK( !r2.contains( point( 0, 3 ) ) );
}

TEST_CASE( "box_shrinks", "[point]" )
{
    half_open_box b( tripoint_zero, tripoint( 3, 3, 3 ) );
    CAPTURE( b );
    CHECK( b.contains( tripoint( 1, 0, 0 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( b.contains( tripoint( 2, 1, 2 ) ) );
    b.shrink( tripoint( 1, 1, 0 ) ); // NOLINT(cata-use-named-point-constants)
    CAPTURE( b );
    // Shrank in the x and y directions
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    CHECK( !b.contains( tripoint( 1, 0, 0 ) ) );
    CHECK( !b.contains( tripoint( 2, 1, 2 ) ) );
    // Didn't shrink in the z direction
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    CHECK( b.contains( tripoint( 1, 1, 0 ) ) );
    CHECK( b.contains( tripoint( 1, 1, 2 ) ) );
}

TEST_CASE( "point_to_from_string", "[point]" )
{
    SECTION( "points_from_string" ) {
        CHECK( point_south.to_string() == "(0,1)" );
        CHECK( tripoint( -1, 0, 1 ).to_string() == "(-1,0,1)" );
    }

    SECTION( "point_round_trip" ) {
        point p( 10, -777 );
        CHECK( point::from_string( p.to_string() ) == p );
    }

    SECTION( "tripoint_round_trip" ) {
        tripoint p( 10, -777, 6 );
        CHECK( tripoint::from_string( p.to_string() ) == p );
    }
}

TEST_CASE( "tripoint_xy", "[point]" )
{
    tripoint p( 1, 2, 3 );
    CHECK( p.xy() == point( 1, 2 ) );
}

TEST_CASE( "closest_points_first_tripoint", "[point]" )
{
    const tripoint center = { 1, -1, 2 };

    GIVEN( "min_dist > max_dist" ) {
        const std::vector<tripoint> result = closest_points_first( center, 1, 0 );

        CHECK( result.empty() );
    }

    GIVEN( "min_dist = max_dist = 0" ) {
        const std::vector<tripoint> result = closest_points_first( center, 0, 0 );

        REQUIRE( result.size() == 1 );
        CHECK( result[0] == tripoint{ 1, -1, 2 } );
    }

    GIVEN( "min_dist = 0, max_dist = 1" ) {
        const std::vector<tripoint> result = closest_points_first( center, 0, 1 );

        REQUIRE( result.size() == 9 );
        CHECK( result[0] == tripoint{  1, -1, 2 } );
        CHECK( result[1] == tripoint{  2, -1, 2 } );
        CHECK( result[2] == tripoint{  2,  0, 2 } );
        CHECK( result[3] == tripoint{  1,  0, 2 } );
        CHECK( result[4] == tripoint{  0,  0, 2 } );
        CHECK( result[5] == tripoint{  0, -1, 2 } );
        CHECK( result[6] == tripoint{  0, -2, 2 } );
        CHECK( result[7] == tripoint{  1, -2, 2 } );
        CHECK( result[8] == tripoint{  2, -2, 2 } );
    }

    GIVEN( "min_dist = 2, max_dist = 2" ) {
        const std::vector<tripoint> result = closest_points_first( center, 2, 2 );

        REQUIRE( result.size() == 16 );

        CHECK( result[0]  == tripoint{  3, -2, 2 } );
        CHECK( result[1]  == tripoint{  3, -1, 2 } );
        CHECK( result[2]  == tripoint{  3,  0, 2 } );
        CHECK( result[3]  == tripoint{  3,  1, 2 } );
        CHECK( result[4]  == tripoint{  2,  1, 2 } );
        CHECK( result[5]  == tripoint{  1,  1, 2 } );
        CHECK( result[6]  == tripoint{  0,  1, 2 } );
        CHECK( result[7]  == tripoint{ -1,  1, 2 } );
        CHECK( result[8]  == tripoint{ -1,  0, 2 } );
        CHECK( result[9]  == tripoint{ -1, -1, 2 } );
        CHECK( result[10] == tripoint{ -1, -2, 2 } );
        CHECK( result[11] == tripoint{ -1, -3, 2 } );
        CHECK( result[12] == tripoint{  0, -3, 2 } );
        CHECK( result[13] == tripoint{  1, -3, 2 } );
        CHECK( result[14] == tripoint{  2, -3, 2 } );
        CHECK( result[15] == tripoint{  3, -3, 2 } );
    }
}

TEST_CASE( "closest_points_first_point_abs_omt", "[point]" )
{
    const point_abs_omt center( 1, 3 );

    GIVEN( "min_dist > max_dist" ) {
        const std::vector<point_abs_omt> result = closest_points_first( center, 1, 0 );

        CHECK( result.empty() );
    }

    GIVEN( "min_dist = max_dist = 0" ) {
        const std::vector<point_abs_omt> result = closest_points_first( center, 0, 0 );

        REQUIRE( result.size() == 1 );
        CHECK( result[0].raw() == point{ 1, 3 } );
    }

    GIVEN( "min_dist = 0, max_dist = 1" ) {
        const std::vector<point_abs_omt> result = closest_points_first( center, 0, 1 );

        REQUIRE( result.size() == 9 );
        CHECK( result[0].raw() == point{ 1, 3 } );
        CHECK( result[1].raw() == point{ 2, 3 } );
        CHECK( result[2].raw() == point{ 2, 4 } );
        CHECK( result[3].raw() == point{ 1, 4 } );
        CHECK( result[4].raw() == point{ 0, 4 } );
        CHECK( result[5].raw() == point{ 0, 3 } );
        CHECK( result[6].raw() == point{ 0, 2 } );
        CHECK( result[7].raw() == point{ 1, 2 } );
        CHECK( result[8].raw() == point{ 2, 2 } );
    }
}
