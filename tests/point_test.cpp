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
