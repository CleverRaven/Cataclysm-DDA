#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"

static const field_type_str_id field_fd_acid( "fd_acid" );

TEST_CASE( "throw_activation", "[item]" )
{
    item acid_bomb( "acidbomb" );

    SECTION( "acid_bomb_hits_ground" ) {
        acid_bomb.activate_thrown( tripoint_zero );
        get_map().get_field( tripoint_zero, field_fd_acid );
        CHECK( get_map().get_field( tripoint_zero, field_fd_acid ) != nullptr );
    }
}
