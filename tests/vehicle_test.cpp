#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "damage.h"
#include "enums.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "optional.h"
#include "activity_actor_definitions.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "veh_appliance.h"
#include "vehicle.h"
#include "veh_type.h"

static const itype_id itype_folded_bicycle( "folded_bicycle" );
static const itype_id itype_folded_inflatable_boat( "folded_inflatable_boat" );
static const itype_id itype_folded_wheelchair_generic( "folded_wheelchair_generic" );
static const itype_id itype_hand_pump( "hand_pump" );

static const itype_id itype_test_extension_cable( "test_extension_cable" );
static const itype_id itype_test_power_cord( "test_power_cord" );

static const vpart_id vpart_ap_test_standing_lamp( "ap_test_standing_lamp" );

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

struct vehicle_preset {
    itype_id vehicle_itype_id; // folding vehicle to test
    std::vector<itype_id> tool_itype_ids; // tool to grant
};

struct damage_preset {
    double damage;
    double degradation;
    double expect_damage;
    double expect_degradation;
    double expect_hp;
};

static void complete_activity( Character &u, const activity_actor &act )
{
    u.assign_activity( player_activity( act ) );
    while( !u.activity.is_null() ) {
        u.set_moves( u.get_speed() );
        u.activity.do_turn( u );
    }
}

static void unfold_and_check( const vehicle_preset &veh_preset, const damage_preset &damage_preset )
{
    map &m = get_map();
    Character &u = get_player_character();

    clear_avatar();
    clear_map();
    clear_vehicles( &m );

    u.worn.wear_item( u, item( "debug_backpack" ), false, false );

    item veh_item( veh_preset.vehicle_itype_id );

    // save these to compare against player folded item later
    const units::volume factory_item_volume = veh_item.volume();
    const units::mass factory_item_weight = veh_item.weight();

    CAPTURE( veh_preset.vehicle_itype_id.str() );
    for( const itype_id &tool_itype_id : veh_preset.tool_itype_ids ) {
        CAPTURE( tool_itype_id.str() );
    }
    CAPTURE( damage_preset.damage, damage_preset.degradation, damage_preset.expect_damage,
             damage_preset.expect_degradation, damage_preset.expect_hp );

    if( !veh_preset.tool_itype_ids.empty() ) {
        INFO( "unfolding vehicle should fail without required tool" );
        {
            complete_activity( u, vehicle_unfolding_activity_actor( veh_item ) );

            // should have no value because avatar has no required tool to unfold
            REQUIRE( !m.veh_at( u.get_location() ).has_value() );

            // the folded item should drop and needs to be deleted from the map
            map_stack map_items = m.i_at( u.pos_bub() );
            REQUIRE( map_items.size() == 1 );
            map_items.clear();
        }
    }

    INFO( "unfolding vehicle item sourced from item factory" );

    // spawn unfolding tools
    for( const itype_id &tool_itype_id : veh_preset.tool_itype_ids ) {
        u.inv->add_item( item( tool_itype_id ) );
    }

    complete_activity( u, vehicle_unfolding_activity_actor( veh_item ) );

    // should succeed now avatar has hand_pump
    optional_vpart_position ovp = m.veh_at( u.get_location() );
    REQUIRE( ovp.has_value() );

    // set damage/degradation on every part
    vehicle &veh = ovp->vehicle();
    for( const vpart_reference &vpr : veh.get_all_parts() ) {
        item base = vpr.part().get_base();
        base.set_degradation( damage_preset.degradation * base.max_damage() );
        base.set_damage( damage_preset.damage * base.max_damage() );
        vpr.part().set_base( base );
        veh.set_hp( vpr.part(), vpr.info().durability, true );
    }

    // fold into an item
    complete_activity( u, vehicle_folding_activity_actor( veh ) );

    // should have no value as vehicle is now folded into item
    REQUIRE( !m.veh_at( u.get_location() ).has_value() );

    // copy the player-folded vehicle item and delete it from the map
    map_stack map_items = m.i_at( u.pos_bub() );
    REQUIRE( map_items.size() == 1 );
    item player_folded_veh = map_items.only_item();
    map_items.clear();

    // check player-folded vehicle has same volume/weight as item factory one
    // may be have some kind of approximation here for packaging/wrappings?
    CHECK( factory_item_volume == player_folded_veh.volume() );
    CHECK( factory_item_weight == player_folded_veh.weight() );

    // unfold the player folded one
    complete_activity( u, vehicle_unfolding_activity_actor( player_folded_veh ) );

    optional_vpart_position ovp_unfolded = m.veh_at( u.get_location() );
    REQUIRE( ovp_unfolded.has_value() );

    // verify the damage/degradation roundtripped via serialization on every part
    for( const vpart_reference &vpr : ovp_unfolded->vehicle().get_all_parts() ) {
        const item &base = vpr.part().get_base();
        CHECK( base.damage() == ( damage_preset.expect_damage * base.max_damage() ) );
        CHECK( base.degradation() == ( damage_preset.expect_degradation * base.max_damage() ) );
        CHECK( vpr.part().health_percent() == damage_preset.expect_hp );
    }

    m.destroy_vehicle( &ovp_unfolded->vehicle() );
}

// Testing iuse::unfold_generic and vehicle part degradation
TEST_CASE( "Unfolding vehicle parts and testing degradation", "[item][degradation][vehicle]" )
{
    std::vector<vehicle_preset> vehicle_presets {
        { itype_folded_inflatable_boat,    { itype_hand_pump } },
        { itype_folded_wheelchair_generic, { } },
        { itype_folded_bicycle,            { } },
    };

    const std::vector<damage_preset> presets {
        { 0.00, 0.00, 0.00, 0.00, 1.00 }, //   0% damaged,   0% degraded
        { 0.25, 0.25, 0.00, 0.25, 1.00 }, //  25% damaged,  25% degraded
        { 0.50, 0.50, 0.25, 0.50, 0.75 }, //  50% damaged,  50% degraded
        { 0.75, 0.50, 0.25, 0.50, 0.75 }, //  75% damaged,  50% degraded
        { 0.75, 1.00, 0.75, 1.00, 0.25 }, //  75% damaged, 100% degraded
        { 1.00, 1.00, 0.75, 1.00, 0.25 }, // 100% damaged, 100% degraded
    };

    for( const vehicle_preset &veh_preset : vehicle_presets ) {
        for( const damage_preset &damage_preset : presets ) {
            unfold_and_check( veh_preset, damage_preset );
        }
    }

    clear_vehicles( &get_map() );
}

// Basically a copy of vehicle::connect() that uses an arbitrary cord type
static void connect_power_line( const tripoint &src_pos, const tripoint &dst_pos,
                                const itype_id &itm )
{
    map &here = get_map();
    item cord( itm );
    cord.set_var( "source_x", src_pos.x );
    cord.set_var( "source_y", src_pos.y );
    cord.set_var( "source_z", src_pos.z );
    cord.set_var( "state", "pay_out_cable" );
    cord.active = true;

    const optional_vpart_position target_vp = here.veh_at( dst_pos );
    const optional_vpart_position source_vp = here.veh_at( src_pos );

    if( !target_vp ) {
        return;
    }
    vehicle *const target_veh = &target_vp->vehicle();
    vehicle *const source_veh = &source_vp->vehicle();
    if( source_veh == target_veh ) {
        return ;
    }

    tripoint target_global = here.getabs( dst_pos );
    const vpart_id vpid( cord.typeId().str() );

    point vcoords = source_vp->mount();
    vehicle_part source_part( vpid, "", vcoords, item( cord ) );
    source_part.target.first = target_global;
    source_part.target.second = target_veh->global_square_location().raw();
    source_veh->install_part( vcoords, source_part );

    vcoords = target_vp->mount();
    vehicle_part target_part( vpid, "", vcoords, item( cord ) );
    tripoint source_global( cord.get_var( "source_x", 0 ),
                            cord.get_var( "source_y", 0 ),
                            cord.get_var( "source_z", 0 ) );
    target_part.target.first = here.getabs( source_global );
    target_part.target.second = source_veh->global_square_location().raw();
    target_veh->install_part( vcoords, target_part );
}

TEST_CASE( "power_cable_stretch_disconnect" )
{
    clear_map();
    clear_avatar();
    map &m = get_map();
    const int max_displacement = 50;
    const cata::optional<item> stand_lamp1( "test_standing_lamp" );
    const cata::optional<item> stand_lamp2( "test_standing_lamp" );

    const tripoint app1_pos( HALF_MAPSIZE_X + 2, HALF_MAPSIZE_Y + 2, 0 );
    const tripoint app2_pos( app1_pos + tripoint( 2, 2, 0 ) );

    place_appliance( app1_pos, vpart_ap_test_standing_lamp, stand_lamp1 );
    place_appliance( app2_pos, vpart_ap_test_standing_lamp, stand_lamp2 );

    optional_vpart_position app1_part = m.veh_at( app1_pos );
    optional_vpart_position app2_part = m.veh_at( app2_pos );
    REQUIRE( app1_part.has_value() );
    REQUIRE( app2_part.has_value() );
    vehicle &app1 = app1_part.part_displayed()->vehicle();
    vehicle &app2 = app2_part.part_displayed()->vehicle();
    REQUIRE( app1.part_count() == 1 );
    REQUIRE( app2.part_count() == 1 );

    GIVEN( "connected via regular power cord" ) {
        connect_power_line( app1_pos, app2_pos, itype_test_power_cord );
        REQUIRE( app1.part_count() == 2 );
        REQUIRE( app2.part_count() == 2 );

        REQUIRE( app1.part( 1 ).get_base().type->maximum_charges() == 3 );

        const int max_dist = app1.part( 1 ).get_base().type->maximum_charges();

        WHEN( "displacing first appliance to the left" ) {
            for( int i = 0; rl_dist( m.getabs( app1.pos_bub() ), m.getabs( app2.pos_bub() ) ) <= max_dist &&
                 i < max_displacement; i++ ) {
                CHECK( app1.part_count() == 2 );
                CHECK( app2.part_count() == 2 );
                m.displace_vehicle( app1, tripoint_west );
                app1.part_removal_cleanup();
                app2.part_removal_cleanup();
            }
            CAPTURE( m.getabs( app1.pos_bub() ) );
            CAPTURE( m.getabs( app2.pos_bub() ) );
            CHECK( app1.part_count() == 1 );
            CHECK( app2.part_count() == 1 );
        }

        WHEN( "displacing second appliance to the right" ) {
            for( int i = 0; rl_dist( m.getabs( app1.pos_bub() ), m.getabs( app2.pos_bub() ) ) <= max_dist &&
                 i < max_displacement; i++ ) {
                CHECK( app1.part_count() == 2 );
                CHECK( app2.part_count() == 2 );
                m.displace_vehicle( app2, tripoint_east );
                app1.part_removal_cleanup();
                app2.part_removal_cleanup();
            }
            CAPTURE( m.getabs( app1.pos_bub() ) );
            CAPTURE( m.getabs( app2.pos_bub() ) );
            CHECK( app1.part_count() == 1 );
            CHECK( app2.part_count() == 1 );
        }
    }

    GIVEN( "connected via extension cable" ) {
        connect_power_line( app1_pos, app2_pos, itype_test_extension_cable );
        REQUIRE( app1.part_count() == 2 );
        REQUIRE( app2.part_count() == 2 );

        REQUIRE( app1.part( 1 ).get_base().type->maximum_charges() == 10 );

        const int max_dist = app1.part( 1 ).get_base().type->maximum_charges();

        WHEN( "displacing first appliance to the left" ) {
            for( int i = 0; rl_dist( m.getabs( app1.pos_bub() ), m.getabs( app2.pos_bub() ) ) <= max_dist &&
                 i < max_displacement; i++ ) {
                CHECK( app1.part_count() == 2 );
                CHECK( app2.part_count() == 2 );
                m.displace_vehicle( app1, tripoint_west );
                app1.part_removal_cleanup();
                app2.part_removal_cleanup();
            }
            CAPTURE( m.getabs( app1.pos_bub() ) );
            CAPTURE( m.getabs( app2.pos_bub() ) );
            CHECK( app1.part_count() == 1 );
            CHECK( app2.part_count() == 1 );
        }

        WHEN( "displacing second appliance to the right" ) {
            for( int i = 0; rl_dist( m.getabs( app1.pos_bub() ), m.getabs( app2.pos_bub() ) ) <= max_dist &&
                 i < max_displacement; i++ ) {
                CHECK( app1.part_count() == 2 );
                CHECK( app2.part_count() == 2 );
                m.displace_vehicle( app2, tripoint_east );
                app1.part_removal_cleanup();
                app2.part_removal_cleanup();
            }
            CAPTURE( m.getabs( app1.pos_bub() ) );
            CAPTURE( m.getabs( app2.pos_bub() ) );
            CHECK( app1.part_count() == 1 );
            CHECK( app2.part_count() == 1 );
        }
    }
}
