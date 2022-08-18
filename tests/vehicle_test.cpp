#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "damage.h"
#include "enums.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "optional.h"
#include "activity_actor_definitions.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "veh_type.h"


static const itype_id itype_folded_inflatable_boat( "folded_inflatable_boat" );
static const itype_id itype_hand_pump( "hand_pump" );

static const vproto_id vehicle_prototype_bicycle( "bicycle" );

TEST_CASE( "detaching_vehicle_unboards_passengers" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    map &here = get_map();
    Character &player_character = get_player_character();
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_bicycle, vehicle_origin, -90_degrees, 0,
                                         0 );
    here.board_vehicle( test_origin, &player_character );
    REQUIRE( player_character.in_vehicle );
    here.detach_vehicle( veh_ptr );
    REQUIRE( !player_character.in_vehicle );
}

TEST_CASE( "destroy_grabbed_vehicle_section" )
{
    GIVEN( "A vehicle grabbed by the player" ) {
        map &here = get_map();
        const tripoint test_origin( 60, 60, 0 );
        avatar &player_character = get_avatar();
        player_character.setpos( test_origin );
        const tripoint vehicle_origin = test_origin + tripoint_south_east;
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_bicycle, vehicle_origin, -90_degrees,
                                             0, 0 );
        REQUIRE( veh_ptr != nullptr );
        tripoint grab_point = test_origin + tripoint_east;
        player_character.grab( object_type::VEHICLE, tripoint_east );
        REQUIRE( player_character.get_grab_type() == object_type::VEHICLE );
        REQUIRE( player_character.grab_point == tripoint_east );
        WHEN( "The vehicle section grabbed by the player is destroyed" ) {
            here.destroy( grab_point );
            REQUIRE( veh_ptr->get_parts_at( grab_point, "", part_status_flag::available ).empty() );
            THEN( "The player's grab is released" ) {
                CHECK( player_character.get_grab_type() == object_type::NONE );
                CHECK( player_character.grab_point == tripoint_zero );
            }
        }
    }
}

TEST_CASE( "add_item_to_broken_vehicle_part" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    vehicle *veh_ptr = get_map().add_vehicle( vehicle_prototype_bicycle, vehicle_origin, 0_degrees,
                       0, 0 );
    REQUIRE( veh_ptr != nullptr );

    const tripoint pos = vehicle_origin + tripoint_west;
    auto cargo_parts = veh_ptr->get_parts_at( pos, "CARGO", part_status_flag::any );
    REQUIRE( !cargo_parts.empty( ) );
    vehicle_part *cargo_part = cargo_parts.front();
    REQUIRE( cargo_part != nullptr );
    //Must not be broken yet
    REQUIRE( !cargo_part->is_broken() );
    //For some reason (0 - cargo_part->hp()) is just not enough to destroy a part
    REQUIRE( veh_ptr->mod_hp( *cargo_part, -( 1 + cargo_part->hp() ), damage_type::BASH ) );
    //Now it must be broken
    REQUIRE( cargo_part->is_broken() );
    //Now part is really broken, adding an item should fail
    const item itm2 = item( "jeans" );
    REQUIRE( !veh_ptr->add_item( *cargo_part, itm2 ) );
}

TEST_CASE( "starting_bicycle_damaged_pedal" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    map &here = get_map();
    Character &player_character = get_player_character();
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_bicycle, vehicle_origin, -90_degrees, 0,
                                         0 );
    here.board_vehicle( test_origin, &player_character );
    REQUIRE( player_character.in_vehicle );
    REQUIRE( veh_ptr->engines.size() == 1 );

    vehicle_part &pedel = veh_ptr->part( veh_ptr->engines[ 0 ] );

    SECTION( "when the pedal has 1/4 hp" ) {
        veh_ptr->set_hp( pedel, pedel.hp() * 0.25, true );
        // Try starting the engine 100 time because it is random that a combustion engine does fails
        for( int i = 0; i < 100 ; i++ ) {
            CHECK( veh_ptr->start_engine( 0 ) );
        }
    }

    SECTION( "when the pedal has 0 hp" ) {
        veh_ptr->set_hp( pedel, 0, true );

        CHECK_FALSE( veh_ptr->start_engine( 0 ) );
    }

    here.detach_vehicle( veh_ptr );
}

static void unfold_and_check( double damage, double degradation,
                              double expect_damage, double expect_degradation, double expected_hp )
{
    map &m = get_map();
    Character &u = get_player_character();
    clear_avatar();
    clear_map();
    clear_vehicles( &m );
    u.move_to( u.get_location() + tripoint( 1, 0, 0 ) );
    item boat_item( itype_folded_inflatable_boat );
    item hand_pump( itype_hand_pump );
    const units::volume boat_item_volume = boat_item.volume();
    const units::mass boat_item_weight = boat_item.weight();
    CAPTURE( damage, degradation, expect_damage, expect_degradation, expected_hp );
    INFO( "unfold inflatable boat sourced from item factory without hand pump" );
    {
        u.assign_activity( player_activity( vehicle_unfolding_activity_actor( boat_item ) ) );
        while( !u.activity.is_null() ) {
            u.set_moves( u.get_speed() );
            u.activity.do_turn( u );
        }

        // should fail as avatar has no hand_pump
        REQUIRE( !m.veh_at( u.get_location() ).has_value() );

        // the folded item should drop and should be deleted
        map_stack map_items = m.i_at( u.pos_bub() );
        REQUIRE( map_items.size() == 1 );
        map_items.clear();
    }

    INFO( "unfold inflatable boat sourced from item factory with hand pump" );
    {
        u.wield( hand_pump ); // give hand_pump

        u.assign_activity( player_activity( vehicle_unfolding_activity_actor( boat_item ) ) );
        while( !u.activity.is_null() ) {
            u.set_moves( u.get_speed() );
            u.activity.do_turn( u );
        }

        // should succeed now avatar has hand_pump
        optional_vpart_position boat_part = m.veh_at( u.get_location() );
        REQUIRE( boat_part.has_value() );

        // set damage/degradation on every part
        vehicle &veh = boat_part->vehicle();
        for( const vpart_reference &vpr : veh.get_all_parts() ) {
            item base = vpr.part().get_base();
            base.set_degradation( degradation * base.max_damage() );
            base.set_damage( damage * base.max_damage() );
            vpr.part().set_base( base );
            veh.set_hp( vpr.part(), vpr.info().durability, true );
        }

        // fold into an item
        u.assign_activity( player_activity( vehicle_folding_activity_actor( veh ) ) );
        while( !u.activity.is_null() ) {
            u.set_moves( u.get_speed() );
            u.activity.do_turn( u );
        }

        // should fail as vehicle is now folded into item
        REQUIRE( !m.veh_at( u.get_location() ).has_value() );

        map_stack map_items = m.i_at( u.pos_bub() );
        REQUIRE( map_items.size() == 1 );
        item player_folded_boat = map_items.only_item();
        map_items.clear();

        // check player-folded boat has same volume/weight as item factory one
        CHECK( boat_item_volume == player_folded_boat.volume() );
        CHECK( boat_item_weight == player_folded_boat.weight() );

        // unfold the player folded one
        u.assign_activity( player_activity( vehicle_unfolding_activity_actor( player_folded_boat ) ) );
        while( !u.activity.is_null() ) {
            u.set_moves( u.get_speed() );
            u.activity.do_turn( u );
        }

        optional_vpart_position unfolded_boat_part = m.veh_at( u.get_location() );
        REQUIRE( unfolded_boat_part.has_value() );

        // verify the damage/degradation roundtripped via serialization on every part
        for( const vpart_reference &vpr : unfolded_boat_part->vehicle().get_all_parts() ) {
            const item &base = vpr.part().get_base();
            CHECK( base.damage() == ( expect_damage * base.max_damage() ) );
            CHECK( base.degradation() == ( expect_degradation * base.max_damage() ) );
            CHECK( vpr.part().health_percent() == expected_hp );
        }
    }
}

// Testing iuse::unfold_generic and vehicle part degradation
TEST_CASE( "Unfolding vehicle parts and testing degradation", "[item][degradation][vehicle]" )
{
    struct degradation_preset {
        double damage;
        double degradation;
        double expect_damage;
        double expect_degradation;
        double expect_hp;
    };

    const std::vector<degradation_preset> presets {
        { 0.00, 0.00, 0.00, 0.00, 1.00 }, //   0% damaged,   0% degraded
        { 0.25, 0.25, 0.00, 0.25, 1.00 }, //  25% damaged,  25% degraded
        { 0.50, 0.50, 0.25, 0.50, 0.75 }, //  50% damaged,  50% degraded
        { 0.75, 0.50, 0.25, 0.50, 0.75 }, //  75% damaged,  50% degraded
        { 0.75, 1.00, 0.75, 1.00, 0.25 }, //  75% damaged, 100% degraded
        { 1.00, 1.00, 0.75, 1.00, 0.25 }, // 100% damaged, 100% degraded
    };

    for( const degradation_preset &p : presets ) {
        unfold_and_check( p.damage, p.degradation, p.expect_damage, p.expect_degradation, p.expect_hp );
    }
}
