#include "cata_catch.h"
#include "cube_direction.h"
#include "omdata.h"

TEST_CASE( "cube_direction_add_om_direction", "[cube_direction]" )
{
    CHECK( cube_direction::north + om_direction::type::north == cube_direction::north );
    CHECK( cube_direction::north + om_direction::type::east == cube_direction::east );
    CHECK( cube_direction::north + om_direction::type::south == cube_direction::south );
    CHECK( cube_direction::north + om_direction::type::west == cube_direction::west );
    CHECK( cube_direction::east + om_direction::type::south == cube_direction::west );
    CHECK( cube_direction::south + om_direction::type::south == cube_direction::north );
    CHECK( cube_direction::west + om_direction::type::south == cube_direction::east );
}

TEST_CASE( "cube_direction_add_int", "[cube_direction]" )
{
    CHECK( cube_direction::north + 0 == cube_direction::north );
    CHECK( cube_direction::north + 1 == cube_direction::east );
    CHECK( cube_direction::north + 2 == cube_direction::south );
    CHECK( cube_direction::north + 3 == cube_direction::west );
    CHECK( cube_direction::north + 4 == cube_direction::north );
    CHECK( cube_direction::east + 2 == cube_direction::west );
    CHECK( cube_direction::south + 2 == cube_direction::north );
    CHECK( cube_direction::west + 2 == cube_direction::east );
}

TEST_CASE( "cube_direction_subtract_int", "[cube_direction]" )
{
    CHECK( cube_direction::north - 0 == cube_direction::north );
    CHECK( cube_direction::north - 1 == cube_direction::west );
    CHECK( cube_direction::north - 2 == cube_direction::south );
    CHECK( cube_direction::north - 3 == cube_direction::east );
    CHECK( cube_direction::north - 4 == cube_direction::north );
    CHECK( cube_direction::east - 2 == cube_direction::west );
    CHECK( cube_direction::south - 2 == cube_direction::north );
    CHECK( cube_direction::west - 2 == cube_direction::east );
}
