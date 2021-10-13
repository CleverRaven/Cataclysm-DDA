#include "cata_catch.h"
#include "player_helpers.h"

static const quality_id qual_BOIL( "BOIL" );

TEST_CASE( "check-tool_qualities", "[tool][quality]" )
{
    CHECK( tool_with_ammo( "mess_kit", 20 ).has_quality( qual_BOIL, 2, 1 ) );
    CHECK( tool_with_ammo( "survivor_mess_kit", 20 ).has_quality( qual_BOIL, 2, 1 ) );
    CHECK( tool_with_ammo( "survivor_mess_kit", 20 ).get_quality( qual_BOIL ) > 0 );
}
