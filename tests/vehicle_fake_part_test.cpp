#include <cmath>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "action.h"
#include "cata_assert.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "pathfinding.h"
#include "player_helpers.h"
#include "point.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const vproto_id vehicle_prototype_bicycle( "bicycle" );
static const vproto_id vehicle_prototype_obstacle_test( "obstacle_test" );
static const vproto_id vehicle_prototype_schoolbus( "schoolbus" );
static const vproto_id vehicle_prototype_suv( "suv" );
static const vproto_id vehicle_prototype_test_van( "test_van" );

static void really_clear_map()
{
    clear_map();
    build_test_map( ter_id( "t_pavement" ) );
}

static int num_fake_parts( const vehicle &veh )
{
    return static_cast<int>( veh.fake_parts.size() );
}

static int num_active_fake_parts( const vehicle &veh )
{
    int ret = 0;
    for( const int fake_index : veh.fake_parts ) {
        cata_assert( fake_index < veh.part_count() );
        ret += veh.part( fake_index ).is_active_fake ? 1 : 0;
    }
    return ret;
}

static void validate_part_count( const vehicle &veh, const int target_velocity,
                                 const units::angle &face_dir, const int real_parts,
                                 const int fake_parts, const int active_fakes )
{
    CAPTURE( veh.disp_name() );
    CAPTURE( veh.velocity );
    CAPTURE( to_degrees( veh.face.dir() ) );
    CAPTURE( to_degrees( face_dir ) );
    if( target_velocity > 0 ) {
        REQUIRE( veh.velocity > 200 );
    }
    REQUIRE( to_degrees( veh.face.dir() ) == Approx( to_degrees( face_dir ) ).epsilon( 0.1f ) );
    CHECK( veh.part_count_real() == real_parts );
    CHECK( num_fake_parts( veh ) == fake_parts );
    CHECK( num_active_fake_parts( veh ) == active_fakes );
}

TEST_CASE( "ensure_fake_parts_enable_on_place", "[vehicle] [vehicle_fake]" )
{
    clear_avatar();
    const int original_parts = 120;
    const int fake_parts = 18;
    std::vector<int> active_fakes_by_angle = { 0, 2, 5, 15, 7, 2 };

    GIVEN( "A vehicle with a known number of parts" ) {
        const tripoint_bub_ms test_origin( 30, 30, 0 );

        for( int quadrant = 0; quadrant < 4; quadrant += 1 ) {
            for( int sub_angle = 0; sub_angle < std::round( 90_degrees / vehicles::steer_increment );
                 sub_angle += 1 )  {
                const units::angle angle = quadrant * 90_degrees + sub_angle * vehicles::steer_increment;
                really_clear_map();
                map &here = get_map();

                vehicle *veh = here.add_vehicle( vehicle_prototype_test_van, test_origin, angle, 100, 0 );
                REQUIRE( veh != nullptr );

                /* since we want all the doors closed anyway, go ahead and test that opening
                 * and closing the real part also changes the fake part
                 */
                // include inactive fakes since the vehicle isn't rotated
                bool tested_a_fake = false;
                for( const vpart_reference &vp : veh->get_all_parts_with_fakes( true ) ) {
                    tested_a_fake |= vp.part().is_fake;
                }
                validate_part_count( *veh, 0, angle, original_parts, fake_parts,
                                     active_fakes_by_angle.at( sub_angle ) );
                REQUIRE( tested_a_fake );
            }
        }
    }
}

TEST_CASE( "ensure_fake_parts_enable_on_turn", "[vehicle] [vehicle_fake]" )
{
    clear_avatar();
    const int original_parts = 120;
    const int fake_parts = 18;
    std::vector<int> active_fakes_by_angle = { 0, 3, 8, 15, 6, 1 };

    GIVEN( "A vehicle with a known number of parts" ) {
        really_clear_map();
        map &here = get_map();
        const tripoint_bub_ms test_origin( 30, 30, 0 );
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_van, test_origin, 0_degrees, 100, 0 );
        REQUIRE( veh != nullptr );

        /* since we want all the doors closed anyway, go ahead and test that opening
         * and closing the real part also changes the fake part
         */
        for( const vpart_reference &vp : veh->get_avail_parts( "OPENABLE" ) ) {
            REQUIRE( !vp.part().is_fake );
            veh->open( here, vp.part_index() );
        }
        // include inactive fakes since the vehicle isn't rotated
        bool tested_a_fake = false;
        for( const vpart_reference &vp : veh->get_all_parts_with_fakes( true ) ) {
            if( vp.info().has_flag( "OPENABLE" ) ) {
                tested_a_fake |= vp.part().is_fake;
                CHECK( vp.part().open );
            }
        }
        REQUIRE( tested_a_fake );
        for( const vpart_reference &vp : veh->get_avail_parts( "OPENABLE" ) ) {
            veh->close( here, vp.part_index() );
        }
        for( const vpart_reference &vp : veh->get_all_parts_with_fakes( true ) ) {
            if( vp.info().has_flag( "OPENABLE" ) ) {
                CHECK( !vp.part().open );
            }
        }

        veh->tags.insert( "IN_CONTROL_OVERRIDE" );
        veh->engine_on = true;
        const int target_velocity = 12 * 100;
        veh->cruise_velocity = target_velocity;
        veh->velocity = veh->cruise_velocity;

        for( int quadrant = 0; quadrant < 4; quadrant += 1 ) {
            for( int sub_angle = 0; sub_angle < std::round( 90_degrees / vehicles::steer_increment );
                 sub_angle += 1 )  {
                const units::angle angle = quadrant * 90_degrees + sub_angle * vehicles::steer_increment;
                here.vehmove();
                validate_part_count( *veh, target_velocity, angle, original_parts, fake_parts,
                                     active_fakes_by_angle.at( sub_angle ) );
                veh->turn( vehicles::steer_increment );
                veh->velocity = veh->cruise_velocity;
            }
        }
        here.vehmove();
        veh->idle( here, true );
        validate_part_count( *veh, target_velocity, 0_degrees, original_parts, fake_parts,
                             active_fakes_by_angle.at( 0 ) );
    }
}

TEST_CASE( "ensure_vehicle_weight_is_constant", "[vehicle] [vehicle_fake]" )
{
    clear_avatar();
    really_clear_map();
    const tripoint_bub_ms test_origin( 30, 30, 0 );
    map &here = get_map();
    vehicle *veh = here.add_vehicle( vehicle_prototype_suv, test_origin, 0_degrees, 0, 0 );
    REQUIRE( veh != nullptr );

    veh->tags.insert( "IN_CONTROL_OVERRIDE" );
    veh->engine_on = true;
    const int target_velocity = 10 * 100;
    veh->cruise_velocity = target_velocity;
    veh->velocity = veh->cruise_velocity;

    GIVEN( "A vehicle with a known weight" ) {
        units::mass initial_weight = veh->total_mass( here );
        WHEN( "The vehicle turns such that it is not perpendicular to a cardinal axis" ) {
            veh->turn( 45_degrees );
            here.vehmove();
            THEN( "The vehicle weight is constant" ) {
                units::mass turned_weight = veh->total_mass( here );
                CHECK( initial_weight == turned_weight );
            }
        }
    }
}

TEST_CASE( "vehicle_collision_applies_damage_to_fake_parent", "[vehicle] [vehicle_fake]" )
{
    clear_avatar();
    really_clear_map();
    map &here = get_map();
    GIVEN( "A moving vehicle traveling at a 45 degree angle to the X axis" ) {
        const tripoint_bub_ms test_origin( 30, 30, 0 );
        vehicle *veh = here.add_vehicle( vehicle_prototype_suv, test_origin, 0_degrees, 100, 0 );
        REQUIRE( veh != nullptr );

        veh->tags.insert( "IN_CONTROL_OVERRIDE" );
        veh->engine_on = true;
        const int target_velocity = 50 * 100;
        veh->cruise_velocity = target_velocity;
        veh->velocity = veh->cruise_velocity;
        veh->turn( 45_degrees );
        here.vehmove();

        WHEN( "A bashable object is placed in the vehicle's path such that it will hit a fake part" ) {
            // we know the mount point of the front right headlight is 2,2
            // that places it's fake mirror at 2,3
            const point_rel_ms fake_r_hl( 2, 3 );
            tripoint_bub_ms fake_front_right_headlight = veh->mount_to_tripoint( &here, fake_r_hl );
            // we're travelling south east, so placing it SE of the fake headlight mirror
            // will impact it on next move
            tripoint_bub_ms obstacle_point = fake_front_right_headlight + tripoint::south_east;
            here.furn_set( obstacle_point.xy(), furn_id( "f_boulder_large" ) );

            int part_count = veh->parts_at_relative( point_rel_ms( 2, 2 ), true, false ).size();
            THEN( "The collision damage is applied to the fake's parent" ) {
                here.vehmove();
                std::vector<int> damaged_parts;
                std::vector<int> damaged_fake_parts;
                // hitting the boulder should have slowed the vehicle down
                REQUIRE( veh->velocity < target_velocity );

                std::vector<int> parent_parts = veh->parts_at_relative( point_rel_ms( 2, 2 ), true, false );
                for( int rel : parent_parts ) {
                    vehicle_part &vp = veh->part( rel );
                    if( vp.info().durability > vp.hp() ) {
                        damaged_parts.push_back( rel );
                    }
                }
                for( int rel : veh->parts_at_relative( point_rel_ms( 2, 3 ), true, false ) ) {
                    vehicle_part &vp = veh->part( rel );
                    if( vp.info().durability > vp.hp() ) {
                        damaged_fake_parts.push_back( rel );
                    }
                }
                // If a part was smashed, we pass,
                if( parent_parts.size() == static_cast<size_t>( part_count ) ) {
                    CHECK( !damaged_parts.empty() );
                }
                CHECK( damaged_fake_parts.empty() );
            }
        }
    }
}

TEST_CASE( "vehicle_to_vehicle_collision", "[vehicle] [vehicle_fake]" )
{
    clear_avatar();
    really_clear_map();
    map &here = get_map();
    GIVEN( "A moving vehicle traveling at a 30 degree angle to the X axis" ) {
        const tripoint_bub_ms test_origin( 30, 30, 0 );
        vehicle *veh = here.add_vehicle( vehicle_prototype_test_van, test_origin, 30_degrees, 100, 0 );
        REQUIRE( veh != nullptr );
        const tripoint_bub_ms global_origin = veh->pos_bub( here );

        veh->tags.insert( "IN_CONTROL_OVERRIDE" );
        veh->engine_on = true;
        const int target_velocity = 50 * 100;
        veh->cruise_velocity = target_velocity;
        veh->velocity = veh->cruise_velocity;
        here.vehmove();
        const tripoint_bub_ms global_move = veh->pos_bub( here );
        const tripoint_bub_ms obstacle_point = test_origin + 2 * ( global_move - global_origin );
        vehicle *trg = here.add_vehicle( vehicle_prototype_schoolbus, obstacle_point, 90_degrees, 100, 0 );
        REQUIRE( trg != nullptr );
        trg->name = "crash bus";
        WHEN( "A vehicle is placed in the vehicle's path such that it will hit a true part" ) {
            // we're travelling south east, so place another vehicle in the way.

            THEN( "The collision damage is applied to the true part" ) {
                std::vector<int> damaged_parts;
                std::vector<int> trg_damaged_parts;

                int moves = 0;
                while( veh->velocity == target_velocity  && moves < 3 ) {
                    here.vehmove();
                    moves += 1;
                }

                // hitting the bus should have slowed the vehicle down
                REQUIRE( veh->velocity < target_velocity );

                for( const vpart_reference &vp : veh->get_all_parts() ) {
                    if( vp.info().durability > vp.part().hp() ) {
                        damaged_parts.push_back( vp.part_index() );
                    }
                }

                for( const vpart_reference &vp : trg->get_all_parts() ) {
                    if( vp.info().durability > vp.part().hp() ) {
                        trg_damaged_parts.push_back( vp.part_index() );
                    }
                }

                CHECK( !damaged_parts.empty() );
                CHECK( !trg_damaged_parts.empty() );
            }
        }
    }
}

TEST_CASE( "ensure_vehicle_with_no_obstacles_has_no_fake_parts", "[vehicle] [vehicle_fake]" )
{
    clear_avatar();
    really_clear_map();
    map &here = get_map();
    GIVEN( "A vehicle with no parts that block movement" ) {
        const tripoint_bub_ms test_origin( 30, 30, 0 );
        vehicle *veh = here.add_vehicle( vehicle_prototype_bicycle, test_origin, 45_degrees, 100, 0 );
        REQUIRE( veh != nullptr );
        WHEN( "The vehicle is placed in the world" ) {
            THEN( "There are no fake parts added" ) {
                validate_part_count( *veh, 0, 45_degrees, veh->part_count(), 0, 0 );
            }
        }
    }
}

TEST_CASE( "vehicle_with_fake_obstacle_parts_block_movement", "[vehicle][vehicle_fake]" )
{
    clear_avatar();
    really_clear_map();
    map &here = get_map();
    Character &you = get_player_character();
    const tripoint_bub_ms test_origin( 30, 30, 0 );
    vehicle *veh = here.add_vehicle( vehicle_prototype_obstacle_test,
                                     test_origin, 315_degrees, 100, 0 );
    REQUIRE( veh != nullptr );
    veh->refresh( );
    here.set_seen_cache_dirty( 0 );
    here.build_map_cache( 0 );
    validate_part_count( *veh, 0, 315_degrees, 11, 6, 5 );
    const tripoint_bub_ms src  = { test_origin - point( 2, 0 ) };
    const tripoint_bub_ms dest = { test_origin + point( 2, 0 ) };
    std::vector<tripoint_bub_ms> route = here.route(
            src, pathfinding_target::point( dest ),
            you.get_pathfinding_settings() );
    REQUIRE( !route.empty() );
    CAPTURE( route );
    REQUIRE( route.size() == 7 );
}

TEST_CASE( "fake_parts_are_opaque", "[vehicle][vehicle_fake]" )
{
    really_clear_map();
    Character &you = get_player_character();
    clear_avatar();
    const tripoint_bub_ms test_origin = you.pos_bub() + point( 6, 2 );
    map &here = get_map();
    set_time_to_day();

    REQUIRE( you.sees( here, you.pos_bub( here ) + point( 10, 10 ) ) );
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_van, test_origin, 315_degrees, 100, 0 );
    REQUIRE( veh != nullptr );
    here.set_seen_cache_dirty( 0 );
    here.build_map_cache( 0 );
    CHECK( !you.sees( here, you.pos_bub( here ) + point( 10, 10 ) ) );
}

TEST_CASE( "open_and_close_fake_doors", "[vehicle][vehicle_fake]" )
{
    really_clear_map();
    Character &you = get_player_character();
    clear_avatar();
    const tripoint_bub_ms test_origin = you.pos_bub() + point( 3, 0 );
    map &here = get_map();

    vehicle *veh = here.add_vehicle( vehicle_prototype_test_van, test_origin, 315_degrees, 100, 0 );
    REQUIRE( veh != nullptr );

    // First get the doors to a known good state.
    for( const vpart_reference &vp : veh->get_avail_parts( "OPENABLE" ) ) {
        REQUIRE( !vp.part().is_fake );
        veh->close( here, vp.part_index() );
    }

    // Then scan through all the openables including fakes and assert that we can open them.
    int fakes_tested = 0;
    for( const vpart_reference &vp : veh->get_all_parts_with_fakes() ) {
        if( vp.info().has_flag( "OPENABLE" ) && vp.part().is_fake ) {
            fakes_tested++;
            REQUIRE( !vp.part().open );
            CHECK( can_interact_at( ACTION_OPEN, here, vp.pos_bub( here ) ) );
            int part_to_open = veh->next_part_to_open( vp.part_index() );
            // This should be the same part for this use case since there are no curtains etc.
            REQUIRE( part_to_open == static_cast<int>( vp.part_index() ) );
            // Using open_all_at because it will usually be from outside the vehicle.
            veh->open_all_at( here, part_to_open );
            CHECK( vp.part().open );
            CHECK( veh->part( vp.part().fake_part_to ).open );
        }
    }
    REQUIRE( fakes_tested == 4 );

    tripoint_bub_ms prev_player_pos = you.pos_bub();
    // Then open them all back up.
    for( const vpart_reference &vp : veh->get_avail_parts( "OPENABLE" ) ) {
        REQUIRE( !vp.part().is_fake );
        veh->open( here, vp.part_index() );
        REQUIRE( vp.part().open );
        if( !vp.part().has_fake ) {
            continue;
        }
        vpart_reference fake_door( *veh, vp.part().fake_part_at );
        if( !fake_door.part().is_active_fake ) {
            continue;
        }
        CAPTURE( prev_player_pos );
        CAPTURE( you.pos_bub( here ) );
        REQUIRE( veh->can_close( vp.part_index(), you ) );
        REQUIRE( veh->can_close( fake_door.part_index(), you ) );
        you.setpos( vp.pos_abs() );
        CHECK( !veh->can_close( vp.part_index(), you ) );
        CHECK( !veh->can_close( fake_door.part_index(), you ) );
        // Move to the location of the fake part and repeat the assetion
        you.setpos( fake_door.pos_abs() );
        you.setpos( fake_door.pos_abs() );
        CHECK( !veh->can_close( vp.part_index(), you ) );
        CHECK( !veh->can_close( fake_door.part_index(), you ) );
        you.setpos( here, prev_player_pos );
    }

    // Then scan through all the openables including fakes and assert that we can close them.
    fakes_tested = 0;
    for( const vpart_reference &vp : veh->get_all_parts_with_fakes() ) {
        if( vp.info().has_flag( "OPENABLE" ) && vp.part().is_fake ) {
            fakes_tested++;
            CHECK( vp.part().open );
            CHECK( can_interact_at( ACTION_CLOSE, here, vp.pos_bub( here ) ) );
            int part_to_close = veh->next_part_to_close( vp.part_index() );
            // This should be the same part for this use case since there are no curtains etc.
            REQUIRE( part_to_close == static_cast<int>( vp.part_index() ) );
            // Using open_all_at because it will usually be from outside the vehicle.
            veh->close( here, part_to_close );
            CHECK( !vp.part().open );
            CHECK( !veh->part( vp.part().fake_part_to ).open );
        }
    }
    REQUIRE( fakes_tested == 4 );
}
