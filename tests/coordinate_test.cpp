#include "catch/catch.hpp"

#include "coordinates.h"
#include "coordinate_conversions.h"
#include "cata_generators.h"
#include "stringmaker.h"

constexpr int num_trials = 5;

TEST_CASE( "coordinate_operations", "[coords]" )
{
    SECTION( "construct_from_raw_point" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        point_ms_abs cp( p );
        CHECK( cp.x() == p.x );
        CHECK( cp.y() == p.y );
    }

    SECTION( "construct_from_raw_tripoint" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        tripoint_ms_abs cp( p );
        CHECK( cp.x() == p.x );
        CHECK( cp.y() == p.y );
        CHECK( cp.z() == p.z );
    }

    SECTION( "construct_from_values" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        {
            point_ms_abs cp( p.x, p.y );
            CHECK( cp.x() == p.x );
            CHECK( cp.y() == p.y );
        }
        {
            tripoint_ms_abs cp( p.x, p.y, p.z );
            CHECK( cp.x() == p.x );
            CHECK( cp.y() == p.y );
            CHECK( cp.z() == p.z );
        }
    }

    SECTION( "addition" ) {
        point p0 = GENERATE( take( num_trials, random_points() ) );
        point p1 = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p0, p1 );
        point_ms_abs abs0( p0 );
        point_ms_rel rel0( p0 );
        point_ms_rel rel1( p1 );
        SECTION( "rel + rel -> rel" ) {
            point_ms_rel sum = rel0 + rel1;
            CHECK( sum.raw() == p0 + p1 );
        }
        SECTION( "abs + rel -> abs" ) {
            point_ms_abs sum = abs0 + rel1;
            CHECK( sum.raw() == p0 + p1 );
        }
        SECTION( "rel + abs -> abs" ) {
            point_ms_abs sum = rel1 + abs0;
            CHECK( sum.raw() == p0 + p1 );
        }
        SECTION( "rel += rel" ) {
            rel0 += rel1;
            CHECK( rel0.raw() == p0 + p1 );
        }
        SECTION( "abs += rel" ) {
            abs0 += rel1;
            CHECK( abs0.raw() == p0 + p1 );
        }
    }

    SECTION( "subtraction" ) {
        point p0 = GENERATE( take( num_trials, random_points() ) );
        point p1 = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p0, p1 );
        point_ms_abs abs0( p0 );
        point_ms_rel rel0( p0 );
        point_ms_rel rel1( p1 );
        SECTION( "rel - rel -> rel" ) {
            point_ms_rel diff = rel0 - rel1;
            CHECK( diff.raw() == p0 - p1 );
        }
        SECTION( "abs - rel -> abs" ) {
            point_ms_abs diff = abs0 - rel1;
            CHECK( diff.raw() == p0 - p1 );
        }
        SECTION( "rel -= rel" ) {
            rel0 -= rel1;
            CHECK( rel0.raw() == p0 - p1 );
        }
        SECTION( "abs -= rel" ) {
            abs0 -= rel1;
            CHECK( abs0.raw() == p0 - p1 );
        }
    }
}

TEST_CASE( "coordinate_comparison", "[coords]" )
{
    SECTION( "compare_points" ) {
        point p0 = GENERATE( take( num_trials, random_points() ) );
        point p1 = GENERATE( take( num_trials, random_points() ) );
        CAPTURE( p0, p1 );
        point_ms_rel cp0( p0 );
        point_ms_rel cp1( p1 );
        CAPTURE( cp0, cp1 );

        CHECK( ( p0 < p1 ) == ( cp0 < cp1 ) );
        CHECK( ( p0 == p1 ) == ( cp0 == cp1 ) );
        CHECK( cp0 == cp0 );
        CHECK( !( cp0 != cp0 ) );
    }

    SECTION( "compare_tripoints" ) {
        tripoint p0 = GENERATE( take( num_trials, random_tripoints() ) );
        tripoint p1 = GENERATE( take( num_trials, random_tripoints() ) );
        CAPTURE( p0, p1 );
        tripoint_ms_rel cp0( p0 );
        tripoint_ms_rel cp1( p1 );
        CAPTURE( cp0, cp1 );

        CHECK( ( p0 < p1 ) == ( cp0 < cp1 ) );
        CHECK( ( p0 == p1 ) == ( cp0 == cp1 ) );
        CHECK( cp0 == cp0 );
        CHECK( !( cp0 != cp0 ) );
    }
}

TEST_CASE( "coordinate_hash", "[coords]" )
{
    SECTION( "point_hash" ) {
        point p = GENERATE( take( num_trials, random_points() ) );
        point_ms_abs cp( p );
        CHECK( std::hash<point_ms_abs>()( cp ) == std::hash<point>()( p ) );
    }

    SECTION( "tripoint_hash" ) {
        tripoint p = GENERATE( take( num_trials, random_tripoints() ) );
        tripoint_ms_abs cp( p );
        CHECK( std::hash<tripoint_ms_abs>()( cp ) == std::hash<tripoint>()( p ) );
    }
}

TEST_CASE( "coordinate_conversion_consistency", "[coords]" )
{
    // Verifies that the new coord_point-based conversions yield the same
    // results as the legacy conversion functions.
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
