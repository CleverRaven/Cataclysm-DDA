#include <string>
#include <utility>

#include "cata_catch.h"
#include "coordinates.h"
#include "point.h"
#include "scenario.h"
#include "start_location.h"
#include "type_id.h"

static const start_location_id start_location_sloc_ocean_shore( "sloc_ocean_shore" );
static const string_id<scenario> scenario_beach_day( "beach_day" );

TEST_CASE( "Test_origin_offset" )
{
    const point_rel_om &offset = scenario_beach_day.obj().get_origin_offset();
    const start_location &beach = start_location_sloc_ocean_shore.obj();
    const point_abs_om origin = point_abs_om::zero + offset;

    tripoint_abs_omt omtstart = tripoint_abs_omt::invalid;

    auto ret = beach.find_player_initial_location( origin );
    omtstart = ret.first;

    CHECK( !omtstart.is_invalid() );
}
