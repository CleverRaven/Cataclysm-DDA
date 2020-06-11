#include <memory>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "damage.h"
#include "enums.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "optional.h"
#include "point.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "veh_type.h"

TEST_CASE( "detaching_vehicle_unboards_passengers" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    vehicle *veh_ptr = g->m.add_vehicle( vproto_id( "bicycle" ), vehicle_origin, -90, 0, 0 );
    g->m.board_vehicle( test_origin, &g->u );
    REQUIRE( g->u.in_vehicle );
    g->m.detach_vehicle( veh_ptr );
    REQUIRE( !g->u.in_vehicle );
}

TEST_CASE( "destroy_grabbed_vehicle_section" )
{
    GIVEN( "A vehicle grabbed by the player" ) {
        const tripoint test_origin( 60, 60, 0 );
        g->place_player( test_origin );
        const tripoint vehicle_origin = test_origin + tripoint_south_east;
        vehicle *veh_ptr = g->m.add_vehicle( vproto_id( "bicycle" ), vehicle_origin, -90, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        tripoint grab_point = test_origin + tripoint_east;
        g->u.grab( object_type::VEHICLE, grab_point );
        REQUIRE( g->u.get_grab_type() != object_type::NONE );
        REQUIRE( g->u.grab_point == grab_point );
        WHEN( "The vehicle section grabbed by the player is destroyed" ) {
            g->m.destroy( grab_point );
            REQUIRE( veh_ptr->get_parts_at( grab_point, "", part_status_flag::available ).empty() );
            THEN( "The player's grab is released" ) {
                CHECK( g->u.get_grab_type() == object_type::NONE );
                CHECK( g->u.grab_point == tripoint_zero );
            }
        }
    }
}

TEST_CASE( "add_item_to_broken_vehicle_part" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    vehicle *veh_ptr = g->m.add_vehicle( vproto_id( "bicycle" ), vehicle_origin, 0, 0, 0 );
    REQUIRE( veh_ptr != nullptr );

    const tripoint pos = vehicle_origin + tripoint_west;
    auto cargo_parts = veh_ptr->get_parts_at( pos, "CARGO", part_status_flag::any );
    REQUIRE( !cargo_parts.empty( ) );
    vehicle_part *cargo_part = cargo_parts.front();
    REQUIRE( cargo_part != nullptr );
    //Must not be broken yet
    REQUIRE( !cargo_part->is_broken() );
    //For some reason (0 - cargo_part->hp()) is just not enough to destroy a part
    REQUIRE( veh_ptr->mod_hp( *cargo_part, -( 1 + cargo_part->hp() ), DT_BASH ) );
    //Now it must be broken
    REQUIRE( cargo_part->is_broken() );
    //Now part is really broken, adding an item should fail
    const item itm2 = item( "jeans" );
    REQUIRE( !veh_ptr->add_item( *cargo_part, itm2 ) );
}

int count_all_parts_plus_active_fakes( const vehicle *veh )
{
    int active_parts = 0;
    for( const auto &p : veh->get_all_parts_incl_fake( false ) ) {
        ++active_parts;
    }
    return active_parts;
}

int count_all_parts( const vehicle *veh )
{
    int all_parts = 0;
    for( const auto &p : veh->get_all_parts_incl_fake( true ) ) {
        ++all_parts;
    }
    return all_parts;
}

int count_original_parts( const vehicle *veh )
{
    int parts = 0;
    for( const auto &p : veh->get_all_parts() ) {
        ++parts;
    }
    return parts;
}

void close_all_doors( vehicle *veh )
{
    for( const vpart_reference vp : veh->get_avail_parts( "OPENABLE" ) ) {
        veh->close( vp.part_index() );
    }
}

TEST_CASE( "ensure_fake_parts_enable_on_turn", "[vehicle] [vehicle_fake]" )
{
    /**
     * 90 parts. 6 tiles long, 5 tiles wide. the four corners get two fake parts each
     * all in all we expect there to be 22 fake parts.
     * If we turn the SUV at a 45 degree angle, the maximum open gaps along it's sides will open
     * in that case we expect 12 fake parts to be enabled.
     */

    int original_parts = 90;
    int fake_parts = 22;
    int enabled_fakes_at_45_degrees = 12;
    int enabled_fakes_at_60_degrees = 6;
    GIVEN( "A vehicle with a known number of parts" ) {
        clear_map();
        const tripoint test_origin( 60, 60, 0 );
        vehicle *veh = g->m.add_vehicle( vproto_id( "suv" ), test_origin, 0, 0, 0 );
        REQUIRE( veh != nullptr );
        close_all_doors( veh );

        veh->tags.insert( "IN_CONTROL_OVERRIDE" );
        veh->engine_on = true;
        const int target_velocity = std::min( 70 * 100, veh->safe_ground_velocity( false ) );
        veh->cruise_velocity = target_velocity;
        veh->velocity = veh->cruise_velocity;


        int active_fake_parts_pre_turn = count_all_parts_plus_active_fakes( veh );
        CHECK( ( active_fake_parts_pre_turn - original_parts ) == 0 );
        int parts = count_original_parts( veh );
        CHECK( parts == original_parts );

        WHEN( "The vehicle turns such that it is not perpendicular to a cardinal axis" ) {
            veh->turn( 45 );
            g->m.vehmove();
            veh->idle( true );

            THEN( "The gaps in the vehicle is filled by fake parts" ) {
                int all_parts = count_all_parts( veh );
                CHECK( all_parts == ( original_parts + fake_parts ) );
                int active_parts = count_all_parts_plus_active_fakes( veh );
                CHECK( active_parts == ( original_parts + enabled_fakes_at_45_degrees ) );
            }
        }
        WHEN( "The vehicle adjust course 15 degrees" ) {
            veh->turn( 60 );
            g->m.vehmove();
            veh->idle( true );
            THEN( "Fakes that no longer fill gaps become inactive" ) {
                int active_parts2 = count_all_parts_plus_active_fakes( veh );
                CHECK( active_parts2 == ( original_parts + enabled_fakes_at_60_degrees ) );
            }
        }
    }
}

TEST_CASE( "ensure_vehicle_weight_is_constant", "[vehicle] [vehicle_fake]" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    vehicle *veh = g->m.add_vehicle( vproto_id( "suv" ), test_origin, 0, 0, 0 );
    REQUIRE( veh != nullptr );
    close_all_doors( veh );

    veh->tags.insert( "IN_CONTROL_OVERRIDE" );
    veh->engine_on = true;
    const int target_velocity = std::min( 70 * 100, veh->safe_ground_velocity( false ) );
    veh->cruise_velocity = target_velocity;
    veh->velocity = veh->cruise_velocity;

    GIVEN( "A vehicle with a known weight" ) {
        units::mass initial_weight = veh->total_mass();
        WHEN( "The vehicle turns such that it is not perpendicular to a cardinal axis" ) {
            veh->turn( 45 );
            g->m.vehmove();
            veh->idle( true );
            THEN( "The vehicle weight is constant" ) {
                units::mass turned_weight = veh->total_mass();

                CHECK( initial_weight == turned_weight );
            }
        }
    }
}

TEST_CASE( "vehicle_collision_applies_damage_to_fake_parent", "[vehicle] [vehicle_fake]" )
{
    clear_map();

    GIVEN( "A moving vehicle traveling at a 45 degree angle to the X axis" ) {
        const tripoint test_origin( 60, 60, 0 );
        vehicle *veh = g->m.add_vehicle( vproto_id( "suv" ), test_origin, 0, 0, 0 );
        REQUIRE( veh != nullptr );
        close_all_doors( veh );

        veh->tags.insert( "IN_CONTROL_OVERRIDE" );
        veh->engine_on = true;
        const int target_velocity = std::min( 70 * 100, veh->safe_ground_velocity( false ) );
        veh->cruise_velocity = target_velocity;
        veh->velocity = veh->cruise_velocity;
        veh->turn( 45 );
        g->m.vehmove();

        WHEN( "A bashable object is placed in the vehicle's path such that it will hit a fake part" ) {
            // we know the mount point of the front right headlight is 2,2
            // that places it's fake mirror at 2,3
            const point fake_r_hl( 2, 3 );
            tripoint fake_front_right_headlight = veh->mount_to_tripoint( fake_r_hl );
            // we're travelling south east, so placing it SE of the fake headlight mirror will impact it on next move
            tripoint obstacle_point = fake_front_right_headlight + tripoint_south_east;
            g->m.furn_set( point( obstacle_point.x, obstacle_point.y ), furn_id( "f_boulder_large" ) );

            THEN( "The collision damage is applied to the fake's parent" ) {
                g->m.vehmove();
                std::vector<int> damaged_parts;
                std::vector<int> damaged_fake_parts;

                for( int rel : veh->parts_at_relative( point( 2, 2 ), true, false ) ) {
                    vehicle_part &vp = veh->part( rel );
                    if( vp.info().durability > vp.hp() ) {
                        damaged_parts.push_back( rel );
                    }
                }
                for( int rel : veh->parts_at_relative( point( 2, 3 ), true, false ) ) {
                    vehicle_part &vp = veh->part( rel );
                    if( vp.info().durability > vp.hp() ) {
                        damaged_fake_parts.push_back( rel );
                    }
                }
                CHECK( damaged_parts.size() > 0 );
                CHECK( damaged_fake_parts.size() == 0 );

            }
        }
    }
}

TEST_CASE( "ensure_vehicle_with_no_obstacles_has_no_fake_parts", "[vehicle] [vehicle_fake]" )
{
    clear_map();

    GIVEN( "A vehicle with no parts that block movement" ) {
        const tripoint test_origin( 60, 60, 0 );
        vehicle *veh = g->m.add_vehicle( vproto_id( "bicycle" ), test_origin, 0, 0, 0 );
        REQUIRE( veh != nullptr );
        WHEN( "The vehicle is placed in the world" ) {
            THEN( "There are no fake parts added" ) {
                int original_parts = count_original_parts( veh );
                int fake_parts = count_all_parts( veh );

                CHECK( ( original_parts - fake_parts ) == 0 );
            }
        }
    }
}
