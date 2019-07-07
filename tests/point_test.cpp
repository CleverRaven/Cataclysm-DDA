#include "catch/catch.hpp"

#include "point.h"

TEST_CASE( "point_to_string", "[point]" )
{
    CHECK( point( 0, 1 ).to_string() == "(0,1)" );
    CHECK( tripoint( -1, 0, 1 ).to_string() == "(-1,0,1)" );
}
