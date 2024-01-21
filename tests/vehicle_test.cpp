#include <optional>
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
#include "activity_actor_definitions.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "veh_appliance.h"
#include "vehicle.h"
#include "veh_type.h"

static const damage_type_id damage_pure( "pure" );

static const itype_id itype_folded_bicycle( "folded_bicycle" );
static const itype_id itype_folded_inflatable_boat( "folded_inflatable_boat" );
static const itype_id itype_folded_wheelchair_generic( "folded_wheelchair_generic" );
static const itype_id itype_hand_pump( "hand_pump" );

static const itype_id itype_test_extension_cable( "test_extension_cable" );
static const itype_id itype_test_power_cord( "test_power_cord" );

static const vpart_id vpart_ap_test_standing_lamp( "ap_test_standing_lamp" );
static const vpart_id vpart_bike_rack( "bike_rack" );
static const vpart_id vpart_programmable_autopilot( "programmable_autopilot" );

static const vproto_id vehicle_prototype_bicycle( "bicycle" );
static const vproto_id vehicle_prototype_car( "car" );
static const vproto_id vehicle_prototype_wheelchair( "wheelchair" );

TEST_CASE( "detaching_vehicle_unboards_passengers", "[vehicle]" )
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

TEST_CASE( "destroy_grabbed_vehicle_section", "[vehicle]" )
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

TEST_CASE( "add_item_to_broken_vehicle_part", "[vehicle]" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    vehicle *veh_ptr = get_map().add_vehicle( vehicle_prototype_bicycle, vehicle_origin, 0_degrees,
                       0, 0 );
    REQUIRE( veh_ptr != nullptr );

    const tripoint pos = vehicle_origin + tripoint_west;
    const std::optional<vpart_reference> ovp_cargo = get_map().veh_at( pos ).cargo();
    REQUIRE( ovp_cargo );
    //Must not be broken yet
    REQUIRE( !ovp_cargo->part().is_broken() );
    //For some reason (0 - cargo_part->hp()) is just not enough to destroy a part
    REQUIRE( veh_ptr->mod_hp( ovp_cargo->part(), -( 1 + ovp_cargo->part().hp() ) ) );
    //Now it must be broken
    REQUIRE( ovp_cargo->part().is_broken() );
    //Now part is really broken, adding an item should fail
    const item itm2 = item( "jeans" );
    REQUIRE( !veh_ptr->add_item( ovp_cargo->part(), itm2 ) );
}

TEST_CASE( "starting_bicycle_damaged_pedal", "[vehicle]" )
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
            CHECK( veh_ptr->start_engine( pedel ) );
        }
    }

    SECTION( "when the pedal has 0 hp" ) {
        veh_ptr->set_hp( pedel, 0, true );

        CHECK_FALSE( veh_ptr->start_engine( pedel ) );
    }

    here.detach_vehicle( veh_ptr );
}

struct vehicle_preset {
    itype_id vehicle_itype_id; // folding vehicle to test
    std::vector<itype_id> tool_itype_ids; // tool to grant
};

struct damage_preset {
    int damage;
    int degradation;
    int expect_damage;
    int expect_degradation;
    int expect_hp;
};

static void complete_activity( Character &u, const activity_actor &act )
{
    u.assign_activity( act );
    while( !u.activity.is_null() ) {
        u.set_moves( u.get_speed() );
        u.activity.do_turn( u );
    }
}

static void spawn_tools_nearby( map &m, Character &u, const vehicle_preset &veh_preset )
{
    map_stack spot = m.i_at( u.pos_bub() + tripoint_north );
    for( const itype_id &tool_itype_id : veh_preset.tool_itype_ids ) {
        spot.insert( item( tool_itype_id ) );
    }
    u.invalidate_crafting_inventory();
}

static void clear_spawned_tools( map &m, Character &u )
{
    m.i_at( u.pos_bub() + tripoint_north ).clear();
    u.invalidate_crafting_inventory();
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

    spawn_tools_nearby( m, u, veh_preset );
    complete_activity( u, vehicle_unfolding_activity_actor( veh_item ) );
    clear_spawned_tools( m, u );

    // should succeed now avatar has hand_pump
    optional_vpart_position ovp = m.veh_at( u.get_location() );
    REQUIRE( ovp.has_value() );

    // set damage/degradation on every part
    vehicle &veh = ovp->vehicle();
    for( const vpart_reference &vpr : veh.get_all_parts() ) {
        vehicle_part &vp = vpr.part();
        item base = vp.get_base();
        base.set_degradation( damage_preset.degradation );
        base.set_damage( damage_preset.damage );
        vp.set_base( std::move( base ) );
    }

    // fold into an item
    spawn_tools_nearby( m, u, veh_preset );
    complete_activity( u, vehicle_folding_activity_actor( veh ) );
    clear_spawned_tools( m, u );

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
    spawn_tools_nearby( m, u, veh_preset );
    complete_activity( u, vehicle_unfolding_activity_actor( player_folded_veh ) );
    clear_spawned_tools( m, u );

    optional_vpart_position ovp_unfolded = m.veh_at( u.get_location() );
    REQUIRE( ovp_unfolded.has_value() );

    // verify the damage/degradation roundtripped via serialization on every part
    for( const vpart_reference &vpr : ovp_unfolded->vehicle().get_all_parts() ) {
        vehicle_part &vp = vpr.part();
        CAPTURE( vp.name() );
        const item &base = vp.get_base();
        const bool expect_degradation = base.type->degrade_increments() > 0;
        if( expect_degradation ) {
            CHECK( base.damage() == damage_preset.expect_damage );
            CHECK( base.degradation() == damage_preset.expect_degradation );
        }
        CHECK( ( vp.max_damage() - vp.damage() ) == damage_preset.expect_hp );
    }

    m.destroy_vehicle( &ovp_unfolded->vehicle() );
}

// Testing iuse::unfold_generic and vehicle part degradation
TEST_CASE( "Unfolding_vehicle_parts_and_testing_degradation", "[item][degradation][vehicle]" )
{
    std::vector<vehicle_preset> vehicle_presets {
        { itype_folded_inflatable_boat,    { itype_hand_pump } },
        { itype_folded_wheelchair_generic, { } },
        { itype_folded_bicycle,            { } },
    };

    const std::vector<damage_preset> presets {
        {    0,    0,    0,    0, 4000 },
        { 1000, 1000, 1000, 1000, 3000 },
        { 2000, 2000, 2000, 2000, 2000 },
        { 2500, 1500, 2500, 1500, 1500 },
        { 3999, 3999, 3999, 3999,    1 },
    };

    for( const vehicle_preset &veh_preset : vehicle_presets ) {
        for( const damage_preset &damage_preset : presets ) {
            unfold_and_check( veh_preset, damage_preset );
        }
    }

    clear_vehicles( &get_map() );
}

struct folded_item_damage_preset {
    itype_id folded_vehicle_item;
    int item_damage_first_fold;
    int item_damage_second_fold;
    int part_damage_second_unfold; // sum of damage over all parts
    int part_damage_third_unfold;  // sum of damage over all parts
};

static void check_folded_item_to_parts_damage_transfer( const folded_item_damage_preset &preset )
{
    CAPTURE( preset.folded_vehicle_item.str(),
             preset.item_damage_first_fold, preset.item_damage_second_fold,
             preset.part_damage_second_unfold, preset.part_damage_third_unfold );

    // exact damage numbers are checked against, there should be almost no rng,
    // only the part damage is pseudo-random spread, while total damage should
    // round trip well in integers
    clear_avatar();
    clear_map();

    map &m = get_map();
    Character &u = get_player_character();

    u.worn.wear_item( u, item( "debug_backpack" ), false, false );

    item veh_item( preset.folded_vehicle_item );

    // unfold fresh item factory item
    complete_activity( u, vehicle_unfolding_activity_actor( veh_item ) );

    optional_vpart_position ovp = m.veh_at( u.get_location() );
    REQUIRE( ovp.has_value() );

    // don't actually need point_north but damage_all filters out direct damage
    // do some damage so it is transferred when folding
    ovp->vehicle().damage_all( 100, 100, damage_pure, ovp->mount() + point_north );

    // fold vehicle into an item
    complete_activity( u, vehicle_folding_activity_actor( ovp->vehicle() ) );

    ovp = m.veh_at( u.get_location() );
    REQUIRE( !ovp.has_value() );

    // copy the player-folded vehicle item and delete it from the map
    map_stack map_items = m.i_at( u.pos_bub() );
    REQUIRE( map_items.size() == 1 );
    item player_folded_veh = map_items.only_item();
    map_items.clear();

    // check the damage was transferred from parts to folded item
    CHECK( player_folded_veh.damage() == preset.item_damage_first_fold );
    CHECK( player_folded_veh.get_var( "avg_part_damage", 0.0 ) == preset.item_damage_first_fold );

    complete_activity( u, vehicle_unfolding_activity_actor( player_folded_veh ) );

    ovp = m.veh_at( u.get_location() );
    REQUIRE( ovp.has_value() );

    int part_damage_before = 0;
    for( const vpart_reference &vpr : ovp->vehicle().get_all_parts() ) {
        part_damage_before += vpr.part().damage();
    }

    // check damage correctly transferred from item to vehicle parts
    CHECK( part_damage_before == preset.part_damage_second_unfold );

    complete_activity( u, vehicle_folding_activity_actor( ovp->vehicle() ) );

    ovp = m.veh_at( u.get_location() );
    REQUIRE( !ovp.has_value() );
    map_items = m.i_at( u.pos_bub() );
    REQUIRE( map_items.size() == 1 );
    player_folded_veh = map_items.only_item();
    map_items.clear();

    // check that we don't add extra item damage after folding
    CHECK( player_folded_veh.damage() == preset.item_damage_first_fold );
    CHECK( player_folded_veh.get_var( "avg_part_damage", 0.0 ) == preset.item_damage_first_fold );

    // add some more damage to the item
    player_folded_veh.mod_damage( 300 );

    // unfold and check extra damage gets distributed into vehicleparts
    complete_activity( u, vehicle_unfolding_activity_actor( player_folded_veh ) );
    ovp = m.veh_at( u.get_location() );
    REQUIRE( ovp.has_value() );

    // add up damage on all parts
    int part_damage_after = 0;
    for( const vpart_reference &vpr : ovp->vehicle().get_all_parts() ) {
        part_damage_after += vpr.part().damage();
    }

    {
        INFO( "Checking extra item damage gets distributed to vehicle parts." );
        CHECK( part_damage_after > part_damage_before );
        CHECK( part_damage_after == preset.part_damage_third_unfold );
    }

    complete_activity( u, vehicle_folding_activity_actor( ovp->vehicle() ) );

    REQUIRE( !m.veh_at( u.get_location() ) );
    map_items = m.i_at( u.pos_bub() );
    REQUIRE( map_items.size() == 1 );
    player_folded_veh = map_items.only_item();
    map_items.clear();

    CHECK( player_folded_veh.damage() == preset.item_damage_second_fold );
    CHECK( player_folded_veh.get_var( "avg_part_damage", 0.0 ) == preset.item_damage_second_fold );
}

TEST_CASE( "Check_folded_item_damage_transfers_to_parts_and_vice_versa", "[item][vehicle]" )
{
    std::vector<folded_item_damage_preset> presets {
        { itype_folded_wheelchair_generic, 2111, 2411, 12666, 14466 },
        { itype_folded_bicycle,            1689, 1989, 18582, 21882 },
    };

    for( const folded_item_damage_preset &preset : presets ) {
        check_folded_item_to_parts_damage_transfer( preset );
    }
}

// Basically a copy of vehicle::connect() that uses an arbitrary cord type
static void connect_power_line( const tripoint &src_pos, const tripoint &dst_pos,
                                const itype_id &itm )
{
    map &here = get_map();

    const optional_vpart_position target_vp = here.veh_at( dst_pos );
    const optional_vpart_position source_vp = here.veh_at( src_pos );
    if( !target_vp || !source_vp || &target_vp->vehicle() == &source_vp->vehicle() ) {
        return;
    }

    item cord( itm );
    cord.link_to( target_vp, source_vp, link_state::vehicle_port );
}

TEST_CASE( "power_cable_stretch_disconnect" )
{
    clear_map();
    clear_avatar();
    map &m = get_map();
    const int max_displacement = 50;
    const std::optional<item> stand_lamp1( "test_standing_lamp" );
    const std::optional<item> stand_lamp2( "test_standing_lamp" );

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
        REQUIRE( app1.part( 1 ).get_base().max_link_length() == 3 );

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
        REQUIRE( app1.part( 1 ).get_base().max_link_length() == 10 );

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

struct rack_activation {
    int racking_vehicle_index; // vehicle with the rack
    tripoint rack_pos;         // rack to activate
    int racked_vehicle_index;  // vehicle to rack on it
    bool expect_failure;       // whether this activation is expected to fail
};

// for each preset all vehicles are spawned at specified positions/facings
// then racking activities in rack_orders are executed
// then unracking activities in unrack_orders are executed
struct rack_preset {
    std::vector<vproto_id> vehicles;            // vehicles to spawn, index matching positions/facings
    std::vector<tripoint> positions;            // spawned vehicle position
    std::vector<point> install_racks;           // install racks on first vehicle at these mounts
    std::vector<units::angle> facings;          // spawned vehicle facing
    std::vector<rack_activation> rack_orders;   // racking orders
    std::vector<rack_activation> unrack_orders; // unracking orders
};

static void rack_check( const rack_preset &preset )
{
    REQUIRE( preset.vehicles.size() == preset.positions.size() );
    REQUIRE( preset.vehicles.size() == preset.facings.size() );

    map &m = get_map();
    Character &u = get_player_character();

    clear_avatar();
    clear_map();
    clear_vehicles( &m );

    std::vector<vehicle *> vehs;
    std::vector<std::string> veh_names;

    for( size_t i = 0; i < preset.vehicles.size(); i++ ) {
        CAPTURE( preset.vehicles[i], preset.positions[i], preset.facings[i] );
        vehicle *veh_ptr = m.add_vehicle( preset.vehicles[i], preset.positions[i],
                                          preset.facings[i], 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        veh_ptr->refresh();
        vehs.push_back( veh_ptr );
        veh_names.push_back( veh_ptr->name );
    }

    for( const point &rack_pos : preset.install_racks ) {
        vehs[0]->install_part( rack_pos, vpart_bike_rack );
    }

    for( const rack_activation &rack_act : preset.rack_orders ) {
        CAPTURE( rack_act.racked_vehicle_index, rack_act.racking_vehicle_index, rack_act.rack_pos );

        vehicle &racking_veh = *vehs[rack_act.racking_vehicle_index];
        vehicle &racked_veh = *vehs[rack_act.racked_vehicle_index];

        const auto rack_parts = racking_veh.get_parts_at( rack_act.rack_pos, "BIKE_RACK_VEH",
                                part_status_flag::available );
        REQUIRE( rack_parts.size() == 1 );
        const int rack_idx = racking_veh.index_of_part( rack_parts[0] );
        REQUIRE( rack_idx >= 0 );
        CAPTURE( rack_idx );

        const auto rackables = racking_veh.find_vehicles_to_rack( rack_idx );
        REQUIRE( !rackables.empty() );

        const auto this_rackable = std::find_if( rackables.begin(), rackables.end(),
        [&racked_veh]( const vehicle::rackable_vehicle & rackable ) {
            return rackable.veh == &racked_veh;
        } );
        REQUIRE( this_rackable != rackables.end() );

        std::string error = capture_debugmsg_during( [&u, &racking_veh, &this_rackable]() {
            bikerack_racking_activity_actor racking_actor( racking_veh,
                    *this_rackable->veh, this_rackable->racks );
            // racked_veh, this_rackable->veh and vehs[] element are invalid past this point
            complete_activity( u, racking_actor );
        } );

        if( !rack_act.expect_failure ) {
            CAPTURE( error );
            REQUIRE( error.empty() );
        } else {
            REQUIRE( error ==
                     "vehicle named Foldable wheelchair is already racked on this vehicle"
                     "racking actor failed: failed racking Foldable wheelchair on Station Wagon, "
                     "racks: [82, 81, and 79]." );
        }

        const optional_vpart_position ovp_racked = m.veh_at(
                    preset.positions[rack_act.racked_vehicle_index] );
        REQUIRE( ovp_racked.has_value() );
        if( !rack_act.expect_failure ) {
            REQUIRE( &ovp_racked->vehicle() == &racking_veh );
        } else {
            REQUIRE( &ovp_racked->vehicle() != &racking_veh );
        }
    }

    for( const rack_activation &rack_act : preset.unrack_orders ) {
        const optional_vpart_position ovp_racked = m.veh_at( rack_act.rack_pos );
        REQUIRE( ovp_racked.has_value() );

        const auto rack_parts = ovp_racked->vehicle().get_parts_at( rack_act.rack_pos,
                                "BIKE_RACK_VEH", part_status_flag::available );
        REQUIRE( rack_parts.size() == 1 );
        const int rack_idx = ovp_racked->vehicle().index_of_part( rack_parts[0] );
        REQUIRE( rack_idx >= 0 );
        CAPTURE( rack_idx );

        const auto unrackables = ovp_racked->vehicle().find_vehicles_to_unrack( rack_idx );
        if( rack_act.expect_failure ) {
            REQUIRE( unrackables.empty() );
        } else {
            REQUIRE( !unrackables.empty() );

            const auto this_unrackable = std::find_if( unrackables.begin(), unrackables.end(),
            [&]( const vehicle::unrackable_vehicle & unrackable ) {
                return unrackable.name == veh_names[rack_act.racked_vehicle_index];
            } );
            REQUIRE( this_unrackable != unrackables.end() );

            bikerack_unracking_activity_actor unracking_actor( ovp_racked->vehicle(),
                    this_unrackable->parts, this_unrackable->racks );
            complete_activity( u, unracking_actor );
        }
    }

    for( size_t i = 0; i < preset.vehicles.size(); i++ ) {
        INFO( "despawning vehicle" );
        CAPTURE( preset.vehicles[i], preset.positions[i] );
        const optional_vpart_position ovp = m.veh_at( preset.positions[i] );
        REQUIRE( ovp.has_value() );
        m.destroy_vehicle( &ovp->vehicle() );
    }
}

// Testing vehicle racking and unracking
TEST_CASE( "Racking_and_unracking_tests", "[vehicle][bikerack]" )
{
    std::vector<rack_preset> racking_presets {
        // basic test; rack bike on car, unrack it, everything should succeed
        {
            { vehicle_prototype_car, vehicle_prototype_bicycle },
            { tripoint_zero,         tripoint( -4, 0, 0 ) },
            { point( -3, -1 ), point( -3, 0 ), point( -3, 1 ), point( -3, 2 ) },
            { 0_degrees,             90_degrees },

            { { 0, tripoint( -3, -1, 0 ), 1, false } }, // rack bicycle to car
            { { 0, tripoint( -3, -1, 0 ), 1, false } }, // unrack bicycle from car
        },
        // rack test probing for #60453 and #52079
        // racking vehicles with same name on ( potentially ) same rack should expect failures
        {
            { vehicle_prototype_car, vehicle_prototype_wheelchair, vehicle_prototype_wheelchair },
            { tripoint_zero,         tripoint( -4, 0, 0 ),         tripoint( -4, 1, 0 )         },
            { point( -3, -1 ), point( -3, 0 ), point( -3, 1 ), point( -3, 2 ) },
            { 0_degrees,             0_degrees,                    0_degrees                    },

            // rack both wheelchairs to car, second rack activation should fail with debugmsg
            { { 0, tripoint( -3, 0, 0 ), 1, false }, { 0, tripoint( -3, -1, 0 ), 2, true } },
            // unrack both wheelchairs from car, second rack activation should fail
            { { 0, tripoint( -3, 0, 0 ), 1, false }, { 0, tripoint( -3, -1, 0 ), 2, true } },
        },
    };

    // shift positions to fit map
    for( rack_preset &preset : racking_presets ) {
        const tripoint offset = { 10, 10, 0 };
        for( tripoint &pos : preset.positions ) {
            pos += offset;
        }
        for( rack_activation &rack_act : preset.rack_orders ) {
            rack_act.rack_pos += offset;
        }
        for( rack_activation &rack_act : preset.unrack_orders ) {
            rack_act.rack_pos += offset;
        }
    }

    for( const rack_preset &preset : racking_presets ) {
        rack_check( preset );
    }

    clear_vehicles( &get_map() );
}

static int test_autopilot_moving( const vproto_id &veh_id, const vpart_id &extra_part )
{
    clear_avatar();
    clear_map();
    Character &player_character = get_player_character();
    // Move player somewhere safe
    REQUIRE_FALSE( player_character.in_vehicle );
    player_character.setpos( tripoint_zero );

    const tripoint map_starting_point( 60, 60, 0 );
    map &here = get_map();
    vehicle *veh_ptr = here.add_vehicle( veh_id, map_starting_point, -90_degrees, 100, 0, false );

    REQUIRE( veh_ptr != nullptr );

    vehicle &veh = *veh_ptr;
    if( !extra_part.is_null() ) {
        vehicle_part vp( extra_part, item( extra_part->base_item ) );
        const int part_index = veh.install_part( point_zero, std::move( vp ) );
        REQUIRE( part_index >= 0 );
    }

    veh.autopilot_on = true;
    veh.is_following = true;
    veh.is_patrolling = false;
    veh.engine_on = true;
    veh.refresh();

    int turns_left = 10;
    int tiles_travelled = 0;
    const tripoint starting_point = veh.global_pos3();
    while( veh.engine_on && turns_left > 0 ) {
        turns_left--;
        here.vehmove();
        veh.idle( true );
        // How much it moved
        tiles_travelled += square_dist( starting_point, veh.global_pos3() );
        // Bring it back to starting point to prevent it from leaving the map
        const tripoint displacement = starting_point - veh.global_pos3();
        here.displace_vehicle( veh, displacement );
    }
    return tiles_travelled;
}

TEST_CASE( "autopilot_tests", "[vehicle][autopilot]" )
{
    // spawns vehicle, installs the extra part if it's not null, activates follow and
    // checks if it moves, most of the test is a cutout from vehicle_efficiency_test.cpp
    CHECK( test_autopilot_moving( vehicle_prototype_car, vpart_id::NULL_ID() ) == 0 );
    CHECK( test_autopilot_moving( vehicle_prototype_car, vpart_programmable_autopilot ) == 9 );
}
