#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"

TEST_CASE( "tick_action_triggering", "[item]" )
{
    item chainsaw( "chainsaw_on" );
    chainsaw.active = true;

    // The chainsaw has no fuel and turns off via its tick_action

    chainsaw.process( get_map(), nullptr, tripoint_zero );
    CHECK( chainsaw.typeId().str() == "chainsaw_off" );
    CHECK( chainsaw.active == false );
}
