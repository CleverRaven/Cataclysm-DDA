#include "catch/catch.hpp"

#include "coordinates.h"
#include "coordinate_conversions.h"
#include "cata_generators.h"
#include "stringmaker.h"

constexpr int num_trials = 5;

TEST_CASE( "coordinate_conversion_consistency", "[coords]" )
{
    SECTION( "omt_to_om_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_om_abs new_conversion = project_to<coords::om>( point_omt_abs( p ) );
        point old_conversion = omt_to_om_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "omt_to_om_tripoint" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p );
        tripoint_om_abs new_conversion = project_to<coords::om>( tripoint_omt_abs( p ) );
        tripoint old_conversion = omt_to_om_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "omt_to_om_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_om_abs new_conversion;
        point_omt_om remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::om>( point_omt_abs( p ) );
        point old_conversion = omt_to_om_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "sm_to_omt_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_omt_abs new_conversion = project_to<coords::omt>( point_sm_abs( p ) );
        point old_conversion = sm_to_omt_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "sm_to_omt_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_omt_abs new_conversion;
        point_sm_omt remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::omt>( point_sm_abs( p ) );
        point old_conversion = sm_to_omt_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "sm_to_om_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_om_abs new_conversion = project_to<coords::om>( point_sm_abs( p ) );
        point old_conversion = sm_to_om_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "sm_to_om_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_om_abs new_conversion;
        point_sm_om remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::om>( point_sm_abs( p ) );
        point old_conversion = sm_to_om_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "omt_to_sm_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_sm_abs new_conversion = project_to<coords::sm>( point_omt_abs( p ) );
        point old_conversion = omt_to_sm_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "om_to_sm_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_sm_abs new_conversion = project_to<coords::sm>( point_om_abs( p ) );
        point old_conversion = om_to_sm_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "ms_to_sm_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_sm_abs new_conversion = project_to<coords::sm>( point_ms_abs( p ) );
        point old_conversion = ms_to_sm_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "sm_to_ms_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_ms_abs new_conversion = project_to<coords::ms>( point_sm_abs( p ) );
        point old_conversion = sm_to_ms_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "ms_to_omt_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_omt_abs new_conversion = project_to<coords::omt>( point_ms_abs( p ) );
        point old_conversion = ms_to_omt_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }

    SECTION( "ms_to_omt_remain_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p );
        point_omt_abs new_conversion;
        point_ms_omt remainder;
        std::tie( new_conversion, remainder ) = project_remain<coords::omt>( point_ms_abs( p ) );
        point old_conversion = ms_to_omt_remain( p );
        CHECK( old_conversion == new_conversion.raw() );
        CHECK( p == remainder.raw() );
    }

    SECTION( "omt_to_seg_tripoint" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p );
        tripoint_seg_abs new_conversion = project_to<coords::seg>( tripoint_omt_abs( p ) );
        tripoint old_conversion = omt_to_seg_copy( p );
        CHECK( old_conversion == new_conversion.raw() );
    }
}
