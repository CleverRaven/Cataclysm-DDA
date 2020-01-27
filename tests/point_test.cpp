#include "catch/catch.hpp"

#include "point.h"

#include "stringmaker.h"

TEST_CASE( "rectangle_containment", "[point]" )
{
    rectangle r( point( 0, 0 ), point( 2, 2 ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( !r.contains_half_open( point( 0, -1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r.contains_half_open( point( 0, 0 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r.contains_half_open( point( 0, 1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( !r.contains_half_open( point( 0, 2 ) ) );
    CHECK( !r.contains_half_open( point( 0, 3 ) ) );

    CHECK( !r.contains_inclusive( point( 0, -1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r.contains_inclusive( point( 0, 0 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r.contains_inclusive( point( 0, 1 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( r.contains_inclusive( point( 0, 2 ) ) );
    CHECK( !r.contains_inclusive( point( 0, 3 ) ) );
}

TEST_CASE( "box_shrinks", "[point]" )
{
    box b( tripoint_zero, tripoint( 3, 3, 3 ) );
    CAPTURE( b );
    CHECK( b.contains_half_open( tripoint( 1, 0, 0 ) ) ); // NOLINT(cata-use-named-point-constants)
    CHECK( b.contains_half_open( tripoint( 2, 1, 2 ) ) );
    b.shrink( tripoint( 1, 1, 0 ) ); // NOLINT(cata-use-named-point-constants)
    CAPTURE( b );
    // Shrank in the x and y directions
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    CHECK( !b.contains_half_open( tripoint( 1, 0, 0 ) ) );
    CHECK( !b.contains_half_open( tripoint( 2, 1, 2 ) ) );
    // Didn't shrink in the z direction
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    CHECK( b.contains_half_open( tripoint( 1, 1, 0 ) ) );
    CHECK( b.contains_half_open( tripoint( 1, 1, 2 ) ) );
}

TEST_CASE( "point_to_string", "[point]" )
{
    CHECK( point_south.to_string() == "(0,1)" );
    CHECK( tripoint( -1, 0, 1 ).to_string() == "(-1,0,1)" );
}

TEST_CASE( "tripoint_xy", "[point]" )
{
    tripoint p( 1, 2, 3 );
    CHECK( p.xy() == point( 1, 2 ) );
}

TEST_CASE( "closest_tripoints_first", "[point]" )
{
    const tripoint center = { 1, -1, 2 };

    GIVEN( "radius = 0" ) {
        const std::vector<tripoint> result = closest_tripoints_first( center, 0 );

        CHECK( result.size() == 1 );
        CHECK( result.front() == tripoint{ 1, -1, 2 } );
    }

    GIVEN( "radius = 1" ) {
        const std::vector<tripoint> result = closest_tripoints_first( center, 1 );

        CHECK( result.size() == 9 );
        CHECK( result.front() == tripoint{ 1, -1, 2 } );
    }

    GIVEN( "radius = 2" ) {
        const std::vector<tripoint> result = closest_tripoints_first( center, 2 );

        CHECK( result.size() == 25 );

        CHECK( result[0]  == tripoint{  1, -1, 2 } );
        CHECK( result[1]  == tripoint{  2, -1, 2 } );
        CHECK( result[2]  == tripoint{  2,  0, 2 } );
        CHECK( result[3]  == tripoint{  1,  0, 2 } );
        CHECK( result[4]  == tripoint{  0,  0, 2 } );
        CHECK( result[5]  == tripoint{  0, -1, 2 } );
        CHECK( result[6]  == tripoint{  0, -2, 2 } );
        CHECK( result[7]  == tripoint{  1, -2, 2 } );
        CHECK( result[8]  == tripoint{  2, -2, 2 } );
        CHECK( result[9]  == tripoint{  3, -2, 2 } );
        CHECK( result[10] == tripoint{  3, -1, 2 } );
        CHECK( result[11] == tripoint{  3,  0, 2 } );
        CHECK( result[12] == tripoint{  3,  1, 2 } );
        CHECK( result[13] == tripoint{  2,  1, 2 } );
        CHECK( result[14] == tripoint{  1,  1, 2 } );
        CHECK( result[15] == tripoint{  0,  1, 2 } );
        CHECK( result[16] == tripoint{ -1,  1, 2 } );
        CHECK( result[17] == tripoint{ -1,  0, 2 } );
        CHECK( result[18] == tripoint{ -1, -1, 2 } );
        CHECK( result[19] == tripoint{ -1, -2, 2 } );
        CHECK( result[20] == tripoint{ -1, -3, 2 } );
        CHECK( result[21] == tripoint{  0, -3, 2 } );
        CHECK( result[22] == tripoint{  1, -3, 2 } );
        CHECK( result[23] == tripoint{  2, -3, 2 } );
        CHECK( result[24] == tripoint{  3, -3, 2 } );
    }
}
