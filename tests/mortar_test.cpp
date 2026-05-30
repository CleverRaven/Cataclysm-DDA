#include <cmath>

#include "cata_catch.h"
#include "coordinates.h"
#include "map_scale_constants.h"
#include "mortar.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"

static const mortar_type_id mortar_m224( "m224" );

TEST_CASE( "mortar_minimum_range_and_deflection_error", "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();

    mortar_error error = mortar.minimum_error( 1000 );
    CHECK( error.range == Approx( 15.0 ) );
    CHECK( error.deflection == Approx( 2.0 ) );

    error = mortar.minimum_error( 3500 );
    CHECK( error.range == Approx( 52.5 ) );
    CHECK( error.deflection == Approx( 7.0 ) );
}

TEST_CASE( "mortar_ballistic_multiplier_caps", "[mortar]" )
{
    CHECK( mortar_type::effective_ballistic_multiplier( 9.0 ) == Approx( 9.0 ) );
    CHECK( mortar_type::effective_ballistic_multiplier( 12.0 ) == Approx( 11.0 ) );
    CHECK( mortar_type::effective_ballistic_multiplier( 130.0 ) == Approx( 70.0 ) );
}

TEST_CASE( "mortar_minimum_target_distance_scales_with_range_error", "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();

    CHECK( mortar.minimum_target_distance( 1000, 3.0 ) ==
           MAX_VIEW_DISTANCE + 90 );
    CHECK( mortar.minimum_target_distance( 3500, 70.0 ) == 1000 );
}

TEST_CASE( "mortar_location_error_projects_onto_ballistic_axes", "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();
    const tripoint_abs_ms target( 0, 0, 0 );
    const tripoint_abs_ms mortar_pos( -100, 0, 0 );
    const mortar_location_error location_error{ 50.0, 100.0 };

    mortar_error projected = mortar.project_location_error( mortar_pos, target,
                             tripoint_abs_ms( -100, 0, 0 ), target, location_error );
    CHECK( projected.range == Approx( 50.0 ) );
    CHECK( projected.deflection == Approx( 100.0 ) );

    projected = mortar.project_location_error( mortar_pos, target,
                tripoint_abs_ms( 0, -100, 0 ), target, location_error );
    CHECK( projected.range == Approx( 100.0 ) );
    CHECK( projected.deflection == Approx( 50.0 ) );
}

TEST_CASE( "mortar_dispersion_does_not_exceed_maximum_range", "[mortar]" )
{
    rng_set_engine_seed( 1 );
    const mortar_type &mortar = mortar_m224.obj();
    const tripoint_abs_ms mortar_pos( 0, 0, 0 );
    const tripoint_abs_ms target( mortar.range(), 0, 0 );
    const mortar_error extreme_error{ 100000.0, 100000.0 };

    for( int i = 0; i < 100; ++i ) {
        const tripoint_abs_ms impact = mortar.apply_dispersion( target, mortar_pos, target,
                                        extreme_error );
        const double distance = std::hypot( impact.x() - mortar_pos.x(),
                                           impact.y() - mortar_pos.y() );
        CHECK( distance <= mortar.range() );
    }
}
