#include <cmath>
#include <optional>
#include <tuple>
#include <vector>

#include "calendar.h"
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

    error = mortar.minimum_error( 3490 );
    CHECK( error.range == Approx( 52.35 ) );
    CHECK( error.deflection == Approx( 6.98 ) );
}

TEST_CASE( "mortar_ballistic_multiplier_caps", "[mortar]" )
{
    CHECK( mortar_type::effective_ballistic_multiplier( 9.0 ) == Approx( 9.0 ) );
    CHECK( mortar_type::effective_ballistic_multiplier( 12.0 ) == Approx( 11.0 ) );
    CHECK( mortar_type::effective_ballistic_multiplier( 130.0 ) == Approx( 70.0 ) );
}

TEST_CASE( "mortar_60mm_flight_time_scales_with_distance", "[mortar]" )
{
    rng_set_engine_seed( 1 );
    const mortar_type &mortar = mortar_m224.obj();

    const std::vector<std::tuple<int, int, int>> expected_bounds = {
        { 500, 10, 20 },
        { 1000, 15, 25 },
        { 2000, 25, 40 },
        { 3000, 35, 50 },
        { 3490, 35, 50 },
    };

    for( const std::tuple<int, int, int> &bounds : expected_bounds ) {
        const int distance = std::get<0>( bounds );
        const int minimum_seconds = std::get<1>( bounds );
        const int maximum_seconds = std::get<2>( bounds );
        for( int i = 0; i < 20; ++i ) {
            const int player_seconds = to_seconds<int>( mortar.player_flight_time( distance ) );
            CHECK( player_seconds >= minimum_seconds );
            CHECK( player_seconds <= maximum_seconds );

            const int npc_seconds = to_seconds<int>( mortar.npc_flight_time( distance ) );
            CHECK( npc_seconds >= minimum_seconds );
            CHECK( npc_seconds <= maximum_seconds );
        }
    }
}

TEST_CASE( "mortar_minimum_target_distance_scales_with_range_error", "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();

    CHECK( mortar.minimum_target_distance( 1000, 3.0 ) ==
           MAX_VIEW_DISTANCE + 90 );
    CHECK( mortar.minimum_target_distance( 3490, 70.0 ) == 1000 );
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

TEST_CASE( "mortar_fire_center_is_clamped_to_valid_fire_range", "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();
    const tripoint_abs_ms mortar_pos( 0, 0, 0 );
    const tripoint_abs_ms target( 500, 500, 0 );
    const int minimum_target_distance = 100;

    const std::vector<tripoint_abs_ms> too_close_centers = {
        tripoint_abs_ms( 10, 0, 0 ),
        tripoint_abs_ms( -10, 0, 0 ),
        tripoint_abs_ms( 10, 10, 0 ),
        tripoint_abs_ms( 0, -10, 0 ),
        tripoint_abs_ms( 0, 0, 0 ),
    };
    for( const tripoint_abs_ms &center : too_close_centers ) {
        const tripoint_abs_ms clamped = mortar.clamp_fire_center_to_range( mortar_pos, center,
                                        target, minimum_target_distance );
        CHECK( rl_dist( mortar_pos, clamped ) > minimum_target_distance );
        CHECK( rl_dist( mortar_pos, clamped ) <= mortar.range() );
    }

    const std::vector<tripoint_abs_ms> too_far_centers = {
        tripoint_abs_ms( mortar.range() + 500, 0, 0 ),
        tripoint_abs_ms( -( mortar.range() + 500 ), 0, 0 ),
        tripoint_abs_ms( mortar.range() + 500, mortar.range() + 500, 0 ),
        tripoint_abs_ms( 0, -( mortar.range() + 500 ), 0 ),
    };
    for( const tripoint_abs_ms &center : too_far_centers ) {
        const tripoint_abs_ms clamped = mortar.clamp_fire_center_to_range( mortar_pos, center,
                                        target, minimum_target_distance );
        CHECK( rl_dist( mortar_pos, clamped ) > minimum_target_distance );
        CHECK( rl_dist( mortar_pos, clamped ) <= mortar.range() );
    }
}

TEST_CASE( "mortar_fire_center_clamp_preserves_fallback_axis_for_zero_vector",
           "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();
    const tripoint_abs_ms mortar_pos( 0, 0, 0 );
    const tripoint_abs_ms fire_center = mortar_pos;
    const tripoint_abs_ms fallback_axis_to( 0, 500, 0 );
    const int minimum_target_distance = 100;

    const tripoint_abs_ms clamped = mortar.clamp_fire_center_to_range( mortar_pos,
                                    fire_center, fallback_axis_to, minimum_target_distance );
    CHECK( clamped.x() == mortar_pos.x() );
    CHECK( clamped.y() > minimum_target_distance );
    CHECK( rl_dist( mortar_pos, clamped ) <= mortar.range() );
}

TEST_CASE( "mortar_creeping_axis_points_away_from_spotter", "[mortar]" )
{
    const tripoint_abs_ms target( 1000, 1000, 0 );
    const tripoint_abs_ms mortar_pos( 500, 500, 0 );

    const std::vector<tripoint_abs_ms> spotters = {
        tripoint_abs_ms( 0, 1000, 0 ),
        tripoint_abs_ms( 2000, 1000, 0 ),
        tripoint_abs_ms( 1000, 0, 0 ),
        tripoint_abs_ms( 1000, 2000, 0 ),
        tripoint_abs_ms( 250, 250, 0 ),
    };
    for( const tripoint_abs_ms &spotter : spotters ) {
        const tripoint_abs_ms axis_to = mortar_make_creeping_axis_to( target, spotter,
                                        mortar_pos );
        const point spotter_to_target( target.x() - spotter.x(), target.y() - spotter.y() );
        const point target_to_axis( axis_to.x() - target.x(), axis_to.y() - target.y() );
        CHECK( spotter_to_target.x * target_to_axis.x +
               spotter_to_target.y * target_to_axis.y > 0 );
    }

    const tripoint_abs_ms fallback_axis = mortar_make_creeping_axis_to( target, target,
                                          mortar_pos );
    CHECK( fallback_axis.x() > target.x() );
    CHECK( fallback_axis.y() > target.y() );
}

TEST_CASE( "mortar_creeping_adjustment_offsets_away_from_spotter_geometry", "[mortar]" )
{
    const mortar_error error{ 100.0, 100.0 };
    const std::vector<std::tuple<tripoint_abs_ms, tripoint_abs_ms, tripoint_abs_ms>> cases = {
        { tripoint_abs_ms( 0, 0, 0 ), tripoint_abs_ms( 1000, 0, 0 ), tripoint_abs_ms( 0, 0, 0 ) },
        { tripoint_abs_ms( 0, 0, 0 ), tripoint_abs_ms( 1000, 0, 0 ), tripoint_abs_ms( 2000, 0, 0 ) },
        { tripoint_abs_ms( 0, 0, 0 ), tripoint_abs_ms( 0, 1000, 0 ), tripoint_abs_ms( 0, 2000, 0 ) },
        { tripoint_abs_ms( -500, 250, 0 ), tripoint_abs_ms( 1000, 1000, 0 ), tripoint_abs_ms( 0, 0, 0 ) },
    };

    for( const auto &[mortar_pos, target, spotter] : cases ) {
        const tripoint_abs_ms axis_to = mortar_make_creeping_axis_to( target, spotter,
                                        mortar_pos );
        const mortar_creeping_solution solution = mortar_creeping_adjustment( mortar_pos,
                target, axis_to, spotter, error );
        const point spotter_to_target( target.x() - spotter.x(), target.y() - spotter.y() );
        const point target_to_center( solution.center.x() - target.x(),
                                      solution.center.y() - target.y() );
        CHECK( spotter_to_target.x * target_to_center.x +
               spotter_to_target.y * target_to_center.y > 0 );
    }
}

TEST_CASE( "mortar_fire_solution_without_creeping_uses_target_center", "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();
    const tripoint_abs_ms mortar_pos( 0, 0, 0 );
    const tripoint_abs_ms target( 1000, 0, 0 );
    const tripoint_abs_ms spotter_pos( 0, 0, 0 );
    const tripoint_abs_ms creeping_axis_to( 2000, 0, 0 );
    const mortar_location_error location_error{ 0.0, 0.0 };

    const mortar_fire_solution solution = mortar.make_fire_solution( mortar_pos, target,
                                          spotter_pos, creeping_axis_to, spotter_pos, target,
                                          location_error, 1.0, true, false );

    CHECK( solution.target_distance == 1000 );
    CHECK( solution.minimum_target_distance ==
           mortar.minimum_target_distance( solution.target_distance, 1.0 ) );
    CHECK( solution.minimum_error.range == Approx( mortar.minimum_range_error( 1000 ) ) );
    CHECK( solution.minimum_error.deflection == Approx( mortar.minimum_deflection_error( 1000 ) ) );
    CHECK( solution.ballistic_error.range == Approx( solution.minimum_error.range ) );
    CHECK( solution.ballistic_error.deflection == Approx( solution.minimum_error.deflection ) );
    CHECK( solution.reported_error.range == Approx( solution.ballistic_error.range ) );
    CHECK( solution.reported_error.deflection == Approx( solution.ballistic_error.deflection ) );
    CHECK( solution.fire_center == target );
    CHECK_FALSE( solution.creeping_solution );
}

TEST_CASE( "mortar_fire_solution_clamps_creeping_center_to_valid_range", "[mortar]" )
{
    const mortar_type &mortar = mortar_m224.obj();
    const tripoint_abs_ms mortar_pos( 0, 0, 0 );
    const tripoint_abs_ms target( mortar.range(), 0, 0 );
    const tripoint_abs_ms spotter_pos( mortar.range() - 1000, 0, 0 );
    const tripoint_abs_ms creeping_axis_to( mortar.range() + 1000, 0, 0 );
    const mortar_location_error location_error{ 0.0, 0.0 };

    const mortar_fire_solution solution = mortar.make_fire_solution( mortar_pos, target,
                                          spotter_pos, creeping_axis_to, spotter_pos, target,
                                          location_error, 1.0, true, true );

    REQUIRE( solution.creeping_solution );
    CHECK( solution.creeping_solution->range_limited );
    CHECK( rl_dist( mortar_pos, solution.fire_center ) <= mortar.range() );
    CHECK( rl_dist( mortar_pos, solution.fire_center ) >
           solution.minimum_target_distance );
}

TEST_CASE( "mortar_creeping_adjustment_scales_danger_close_offset", "[mortar]" )
{
    const mortar_error error{ 100.0, 100.0 };
    const tripoint_abs_ms mortar_pos( 0, 0, 0 );
    const tripoint_abs_ms target( 1000, 0, 0 );
    const tripoint_abs_ms axis_to( 2000, 0, 0 );
    const std::vector<std::tuple<tripoint_abs_ms, double, int>> cases = {
        { tripoint_abs_ms( 700, 0, 0 ), 1.0, 100 },
        { tripoint_abs_ms( 850, 0, 0 ), 1.5, 150 },
        { tripoint_abs_ms( 950, 0, 0 ), 2.0, 200 },
    };

    for( const auto &[spotter_pos, expected_multiplier, expected_offset] : cases ) {
        const mortar_creeping_solution solution = mortar_creeping_adjustment( mortar_pos,
                target, axis_to, spotter_pos, error );
        CHECK( solution.offset_multiplier == Approx( expected_multiplier ) );
        CHECK( solution.center.x() - target.x() == expected_offset );
        CHECK( solution.center.y() == target.y() );
    }
}
