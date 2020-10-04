#include "catch/catch.hpp"

#include <array>

#include "damage.h"
#include "type_id.h"
#include "veh_type.h"

TEST_CASE( "verify_copy_from_gets_damage_reduction", "[vehicle]" )
{
    // Picking halfboard_horizontal as a vpart which is likely to remain
    // defined via copy-from, and which should have non-zero damage reduction.
    const vpart_info &vp = vpart_id( "halfboard_horizontal" ).obj();
    CHECK( vp.damage_reduction[static_cast<int>( damage_type::BASH )] != 0 );
}

TEST_CASE( "vehicle_parts_seats_and_beds_have_beltable_flags", "[vehicle][vehicle_parts]" )
{
    // this checks all seats and beds either BELTABLE or NONBELTABLE but not both

    for( const auto &e : vpart_info::all() ) {
        const auto &vp = e.second;

        if( !vp.has_flag( "BED" ) && !vp.has_flag( "SEAT" ) ) {
            continue;
        }
        CAPTURE( vp.get_id().c_str() );
        CHECK( ( vp.has_flag( "BELTABLE" ) ^ vp.has_flag( "NONBELTABLE" ) ) );
    }
}

TEST_CASE( "vehicle_parts_boardable_openable_parts_have_door_flag", "[vehicle][vehicle_parts]" )
{
    // this checks all BOARDABLE and OPENABLE parts have DOOR flag

    for( const auto &e : vpart_info::all() ) {
        const auto &vp = e.second;

        if( !vp.has_flag( "BOARDABLE" ) || !vp.has_flag( "OPENABLE" ) ) {
            continue;
        }
        CAPTURE( vp.get_id().c_str() );
        CHECK( vp.has_flag( "DOOR" ) );
    }
}
