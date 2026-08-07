#include <cmath>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mortar.h"
#include "npc.h"
#include "npctalk.h"
#include "overmapbuffer.h"
#include "pimpl.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"
#include "visitable.h"

static const itype_id itype_60mm_shell_m720a1( "60mm_shell_m720a1" );
static const itype_id itype_rock( "rock" );
static const mortar_type_id mortar_m224( "m224" );
static const oter_str_id oter_field( "field" );
static const ter_str_id ter_t_wall( "t_wall" );

TEST_CASE( "mortar_support_ammo_release_preserves_inventory", "[mortar]" )
{
    standard_npc gunner( "Mortar gunner", tripoint_bub_ms::zero, {}, 4, 8, 8, 8, 10 );
    item issued_round( itype_60mm_shell_m720a1, calendar::turn );
    issued_round.set_var( "mortar_support_ammo", true );
    issued_round.set_var( "test_mortar_identity", "issued" );
    item personal_round( itype_60mm_shell_m720a1, calendar::turn );
    personal_round.set_var( "test_mortar_identity", "personal" );
    gunner.inv->add_item( std::move( issued_round ), false, true, false );
    gunner.inv->add_item( std::move( personal_round ), false, true, false );

    CHECK( talk_effect_fun::release_mortar_ammo( gunner, false ) == 1 );

    int round_count = 0;
    gunner.visit_items( [&round_count]( item * it, item * ) {
        if( it->typeId() == itype_60mm_shell_m720a1 ) {
            round_count += it->count_by_charges() ? it->charges : 1;
            CHECK_FALSE( it->has_var( "mortar_support_ammo" ) );
            CHECK_FALSE( it->get_var( "test_mortar_identity" ).empty() );
        }
        return VisitResponse::NEXT;
    } );
    CHECK( round_count == 2 );
}

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

TEST_CASE( "mortar_impact_payload_rejects_non_ammunition", "[mortar]" )
{
    const item non_ammunition( itype_rock, calendar::turn );
    CHECK_FALSE( mortar_schedule_impact_payload( non_ammunition, tripoint_abs_ms::zero,
                 calendar::turn ) );
}

TEST_CASE( "mortar_deployment_matches_furniture", "[mortar]" )
{
    clear_map();
    map &here = get_map();
    const mortar_type &mortar = mortar_m224.obj();
    const tripoint_bub_ms pos( 60, 60, 0 );
    const tripoint_abs_ms abs_pos = here.get_abs( pos );

    here.furn_set( pos, furn_str_id::NULL_ID() );
    CHECK_FALSE( mortar.is_deployed_at( abs_pos ) );
    here.furn_set( pos, mortar.furniture() );
    CHECK( mortar.is_deployed_at( abs_pos ) );
    here.furn_set( pos, furn_str_id::NULL_ID() );
    CHECK_FALSE( mortar.is_deployed_at( abs_pos ) );
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

TEST_CASE( "mortar_spotter_observation_requires_local_los", "[mortar]" )
{
    clear_map();
    calendar::turn = calendar::turn_zero + 12_hours;
    g->reset_light_level();
    map &here = get_map();
    get_player_character().recalc_sight_limits();
    here.invalidate_visibility_cache();
    here.update_visibility_cache( 0 );
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0 );

    const tripoint_bub_ms spotter_pos( 60, 60, 0 );
    standard_npc spotter( "Mortar spotter", spotter_pos, {}, 4, 8, 8, 8, 10 );
    spotter.recalc_sight_limits();

    const tripoint_abs_omt spotter_omt = spotter.pos_abs_omt();
    for( int dx = 0; dx <= 5; ++dx ) {
        overmap_buffer.ter_set( tripoint_abs_omt( spotter_omt.x() + dx, spotter_omt.y(),
                                spotter_omt.z() ), oter_field.id() );
    }

    const tripoint_abs_ms target = here.get_abs( spotter_pos + tripoint(
                                       MAX_VIEW_DISTANCE + 20, 0, 0 ) );
    REQUIRE_FALSE( here.inbounds( target ) );
    CHECK( mortar_spotter_can_observe( spotter, target ) );

    REQUIRE( here.ter_set( spotter_pos + tripoint::east, ter_t_wall ) );
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
    CHECK_FALSE( mortar_spotter_can_observe( spotter, target ) );
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
                                          location_error, 1.0, false );

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
                                          location_error, 1.0, true );

    REQUIRE( solution.creeping_solution );
    CHECK( solution.creeping_solution->range_limited );
    CHECK( rl_dist( mortar_pos, solution.fire_center ) <= mortar.range() );
    CHECK( rl_dist( mortar_pos, solution.fire_center ) > MAX_VIEW_DISTANCE );
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
