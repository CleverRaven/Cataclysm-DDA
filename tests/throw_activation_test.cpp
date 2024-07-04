#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"

static const field_type_str_id field_fd_fire( "fd_fire" );

TEST_CASE( "throw_activation", "[item]" )
{
    item molotov( "molotov_lit" );

    SECTION( "molotov_hits_ground" ) {
        molotov.activate_thrown( tripoint_zero );
        get_map().get_field( tripoint_zero, field_fd_fire );
        CHECK( get_map().get_field( tripoint_zero, field_fd_fire ) != nullptr );
    }
}
