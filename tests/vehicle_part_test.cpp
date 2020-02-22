#include "catch/catch.hpp"

#include "veh_type.h"
#include "cata_string_consts.h"

TEST_CASE( "verify_copy_from_gets_damage_reduction", "[vehicle]" )
{
    // Picking halfboard_horizontal as a vpart which is likely to remain
    // defined via copy-from, and which should have non-zero damage reduction.
    const vpart_info &vp = vpart_halfboard_horizontal.obj();
    CHECK( vp.damage_reduction[DT_BASH] != 0 );
}
