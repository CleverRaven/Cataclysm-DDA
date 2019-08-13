#include "catch/catch.hpp"

#include "point.h"

#include "stringmaker.h"

TEST_CASE( "rectangle_containment", "[point]" )
{
    rectangle r( point_zero, point( 2, 2 ) );
    CHECK( !r.contains_half_open( point_north ) );
    CHECK( r.contains_half_open( point_zero ) );
    CHECK( r.contains_half_open( point_south ) );
    CHECK( !r.contains_half_open( point( 0, 2 ) ) );
    CHECK( !r.contains_half_open( point( 0, 3 ) ) );

    CHECK( !r.contains_inclusive( point_north ) );
    CHECK( r.contains_inclusive( point_zero ) );
    CHECK( r.contains_inclusive( point_south ) );
    CHECK( r.contains_inclusive( point( 0, 2 ) ) );
    CHECK( !r.contains_inclusive( point( 0, 3 ) ) );
}

TEST_CASE( "box_shrinks", "[point]" )
{
    box b( tripoint_zero, tripoint( 3, 3, 3 ) );
    CAPTURE( b );
    CHECK( b.contains_half_open( tripoint_east ) );
    CHECK( b.contains_half_open( tripoint( 2, 1, 2 ) ) );
    b.shrink( tripoint_south_east );
    CAPTURE( b );
    // Shrank in the x and y directions
    CHECK( !b.contains_half_open( tripoint_east ) );
    CHECK( !b.contains_half_open( tripoint( 2, 1, 2 ) ) );
    // Didn't shrink in the z direction
    CHECK( b.contains_half_open( tripoint_south_east ) );
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
