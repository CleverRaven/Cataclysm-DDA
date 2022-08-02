#include "avatar.h"
#include "cata_catch.h"
#include "effect_on_condition.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"

static const effect_on_condition_id effect_on_condition_EOC_teleport_test( "EOC_teleport_test" );


TEST_CASE( "EOC_teleport", "[eoc]" )
{
    clear_avatar();
    clear_map();
    tripoint_abs_ms before = get_avatar().get_location();
    dialogue newDialog( get_talker_for( get_avatar() ), nullptr );
    effect_on_condition_EOC_teleport_test->activate( newDialog );
    tripoint_abs_ms after = get_avatar().get_location();

    CHECK( before + tripoint_south_east == after );
}
