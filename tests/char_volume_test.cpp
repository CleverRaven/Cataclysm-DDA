#include <memory>
#include <utility>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "mutation.h"
#include "player_helpers.h"
#include "vehicle.h"
#include "veh_type.h"

static const itype_id backpack_giant( "backpack_giant" );
static const itype_id rock_volume_test( "rock_volume_test" );

static const trait_id trait_HUGE( "HUGE" );
static const trait_id trait_LARGE( "LARGE" );
static const trait_id trait_SMALL( "SMALL" );
static const trait_id trait_SMALL2( "SMALL2" );

static const vproto_id veh_character_volume_test_car( "character_volume_test_car" );

static units::volume your_volume_with_trait( trait_id new_trait )
{
    clear_avatar();
    Character &you = get_player_character();
    you.set_mutation( new_trait );
    return you.get_base_volume();
}

static int your_height_with_trait( trait_id new_trait )
{
    clear_avatar();
    Character &you = get_player_character();
    you.set_mutation( new_trait );
    return you.height();
}

TEST_CASE( "character_baseline_volumes", "[volume]" )
{
    clear_avatar();
    Character &you = get_player_character();
    REQUIRE( you.get_mutations().empty() );
    REQUIRE( you.height() == 175 );
    CHECK( you.get_base_volume() == 70_liter );

    REQUIRE( your_height_with_trait( trait_SMALL2 ) == 70 );
    CHECK( your_volume_with_trait( trait_SMALL2 ) == 4480_ml );

    REQUIRE( your_height_with_trait( trait_SMALL ) == 122 );
    CHECK( your_volume_with_trait( trait_SMALL ) == 23717_ml );

    REQUIRE( your_height_with_trait( trait_LARGE ) == 227 );
    CHECK( your_volume_with_trait( trait_LARGE ) == 152778_ml );

    REQUIRE( your_height_with_trait( trait_HUGE ) == 280 );
    CHECK( your_volume_with_trait( trait_HUGE ) == 286720_ml );
}

TEST_CASE( "character_at_volume_can_or_cannot_enter_vehicle", "[volume]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    Character &you = get_player_character();
    REQUIRE( you.get_mutations().empty() );
    REQUIRE( you.get_base_volume() == 70_liter );
    REQUIRE( you.get_total_volume() == 70_liter );

    tripoint test_pos = tripoint{10, 10, 0 };


    clear_vehicles(); // extra safety
    here.add_vehicle( veh_character_volume_test_car, test_pos, 0_degrees, 0, 0 );
    you.setpos( test_pos );
    const optional_vpart_position vp_there = here.veh_at( here.bub_from_abs( you.get_location() ) );
    REQUIRE( vp_there );
    tripoint_abs_ms dest_loc = you.get_location();

    // Empty aisle
    dest_loc = dest_loc + tripoint_north_west;
    bool cramped = false;
    CHECK( you.can_move_to_vehicle_tile( dest_loc, cramped ) );
    CHECK( !cramped );
    dest_loc = you.get_location(); //reset

    // Aisle with 10L rock, a tight fit but not impossible
    dest_loc = dest_loc + tripoint_north;
    CHECK( you.can_move_to_vehicle_tile( dest_loc, cramped ) );
    CHECK( cramped );
    dest_loc = you.get_location(); //reset
    cramped = false;

    // Empty aisle, but we've put on a backpack and a 10L rock in that backpack
    item backpack( backpack_giant );
    auto worn_success = you.wear_item( backpack );
    CHECK( worn_success );
    you.i_add( item( rock_volume_test ) );
    CHECK( 75_liter <= you.get_total_volume() );
    CHECK( you.get_total_volume() <= 100_liter );
    dest_loc = dest_loc + tripoint_north_west;
    CHECK( you.can_move_to_vehicle_tile( dest_loc, cramped ) );
    CHECK( cramped );
    dest_loc = you.get_location(); //reset
    cramped = false;

    // Try the cramped aisle with a rock again, but now we are tiny, so it is easy.
    CHECK( your_volume_with_trait( trait_SMALL2 ) == 4480_ml );
    you.setpos( test_pos ); // set our position again, clear_avatar() moved us
    dest_loc = dest_loc + tripoint_north;
    CHECK( you.can_move_to_vehicle_tile( dest_loc, cramped ) );
    CHECK( !cramped );
    dest_loc = you.get_location(); //reset

    // Same aisle, but now we have HUGE GUTS. We will never fit.
    CHECK( your_volume_with_trait( trait_HUGE ) == 286720_ml );
    you.setpos( test_pos ); // set our position again, clear_avatar() moved us
    dest_loc = dest_loc + tripoint_north;
    CHECK( !you.can_move_to_vehicle_tile( dest_loc ) );
    dest_loc = you.get_location(); //reset

    // And finally, check that our HUGE body won't fit even into an empty aisle.
    dest_loc = dest_loc + tripoint_north_west;
    CHECK( !you.can_move_to_vehicle_tile( dest_loc ) );
}
