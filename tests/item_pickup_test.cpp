#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "rng.h"

static const itype_id itype_backpack_hiking( "backpack_hiking" );
static const itype_id itype_m4_carbine( "m4_carbine" );
static const itype_id itype_rope_6( "rope_6" );

// This test case exists by way of documenting and exhibiting some potentially unexpected behavior
// of the following functions for transferring items into inventory:
//
// - Character::wear_item
// - Character::pick_up
// - Character::i_add
// - item::put_in
//
// namely, that these functions create *copies* of the items, and the original item
// references will not refer to the items placed in inventory.
TEST_CASE( "putting_items_into_inventory_with_put_in_or_i_add", "[pickup][inventory]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map();

    // Spawn items on the map at this location
    const tripoint ground = they.pos();
    item &rope_map = here.add_item( ground, item( itype_rope_6 ) );
    item &backpack_map = here.add_item( ground, item( itype_backpack_hiking ) );

    // Set unique IDs on the items, to verify their copies later
    std::string backpack_uid = random_string( 10 );
    std::string rope_uid = random_string( 10 );
    backpack_map.set_var( "uid", backpack_uid );
    rope_map.set_var( "uid", rope_uid );

    // Ensure avatar does not currently possess these items, or items with their uid
    REQUIRE_FALSE( they.has_item( backpack_map ) );
    REQUIRE_FALSE( they.has_item( rope_map ) );
    REQUIRE_FALSE( character_has_item_with_var_val( they, "uid", backpack_uid ) );
    REQUIRE_FALSE( character_has_item_with_var_val( they, "uid", rope_uid ) );

    WHEN( "avatar wears a hiking backpack from the ground with wear_item" ) {
        they.clear_worn();
        // Get the backpack from the iterator returned by wear_item,
        // for the reference to the backpack that the avatar is wearing now
        std::optional<std::list<item>::iterator> worn = they.wear_item( backpack_map );
        item &backpack = **worn;

        THEN( "they have a copy of the backpack" ) {
            // They have the same type
            CHECK( backpack.typeId() == backpack_map.typeId() );
            // They have the same uid
            CHECK( backpack.get_var( "uid" ) == backpack_uid );
            CHECK( character_has_item_with_var_val( they, "uid", backpack_uid ) );
            // New one is in avatar's possession
            CHECK( they.has_item( backpack ) );
            // Original backpack from the ground is not
            CHECK_FALSE( they.has_item( backpack_map ) );
        }

        WHEN( "using put_in to put a rope directly into the backpack" ) {
            REQUIRE( backpack.put_in( rope_map, item_pocket::pocket_type::CONTAINER ).success() );

            THEN( "the original rope is not in inventory or the backpack" ) {
                CHECK_FALSE( they.has_item( rope_map ) );
                CHECK_FALSE( backpack.has_item( rope_map ) );
            }
            THEN( "they have a copy of the rope in inventory" ) {
                CHECK( character_has_item_with_var_val( they, "uid", rope_uid ) );
            }
            // FIXME: After put_in, there is no way to get the new copied item reference(?)
        }

        WHEN( "using i_add to put the rope into inventory" ) {
            // Add the rope to the inventory (goes in backpack, as it's the only thing worn)
            item_location rope_new = they.i_add( rope_map );

            THEN( "a copy of the rope item is in inventory and in the backpack" ) {
                CHECK( they.has_item( *rope_new ) );
                CHECK( backpack.has_item( *rope_new ) );
                CHECK( character_has_item_with_var_val( they, "uid", rope_uid ) );
            }
            THEN( "the original rope is not in inventory or the backpack" ) {
                CHECK_FALSE( they.has_item( rope_map ) );
                CHECK_FALSE( backpack.has_item( rope_map ) );
            }
        }
    }

    // The Character::pick_up function assigns an item pick-up activity to the character,
    // which can be executed with the process_activity() helper.
    // But Character::pick_up cannot wield or wear items in the act of picking them up;
    // the available storage needs to be worn ahead of time.
    GIVEN( "avatar is not wearing anything that can store items" ) {
        they.clear_worn();

        WHEN( "avatar tries to get the backpack with pick_up" ) {
            item_location backpack_loc( map_cursor( ground ), &backpack_map );
            const drop_locations &pack_droplocs = { std::make_pair( backpack_loc, 1 ) };
            they.pick_up( pack_droplocs );
            process_activity( they );

            THEN( "they fail to acquire the backpack" ) {
                CHECK_FALSE( character_has_item_with_var_val( they, "uid", backpack_uid ) );
            }
        }

        WHEN( "avatar tries to get the rope with pick_up" ) {
            item_location rope_loc( map_cursor( ground ), &rope_map );
            const drop_locations &rope_droplocs = { std::make_pair( rope_loc, 1 ) };
            they.pick_up( rope_droplocs );
            process_activity( they );

            THEN( "they fail to acquire the rope" ) {
                CHECK_FALSE( character_has_item_with_var_val( they, "uid", rope_uid ) );
            }
        }
    }
}

// The below incredibly-specific test case is designed as a regression test for #52422 in which
// picking up items from the ground could result in inventory items being dropped.
//
// One such case is when an "inner container" (container within a container) would be selected as
// the "best pocket" for a picked up item, but inserting the item makes it too big or heavy for its
// outer container, forcing it to be dropped on the ground (along with whatever was inserted).
//
// The reproduction use case here is: Wearing only a backpack containing a rope, when picking up
// an M4 from the ground, the M4 should go into the backpack, not into the rope, and neither the
// rope nor the M4 should be dropped.
TEST_CASE( "pickup_m4_with_a_rope_in_a_hiking_backpack", "[pickup][container]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map();

    // Spawn items on the map at this location
    const tripoint ground = they.pos();
    item &m4a1 = here.add_item( ground, item( itype_m4_carbine ) );
    item &rope_map = here.add_item( ground, item( itype_rope_6 ) );
    item &backpack_map = here.add_item( ground, item( itype_backpack_hiking ) );

    // Ensure that rope and backpack are containers, both capable of holding the M4
    REQUIRE( rope_map.is_container() );
    REQUIRE( backpack_map.is_container() );
    REQUIRE( rope_map.can_contain( m4a1 ).success() );
    REQUIRE( backpack_map.can_contain( m4a1 ).success() );
    REQUIRE( backpack_map.can_contain( rope_map ).success() );

    // Give the M4 a serial number (and the rope too, it's also a deadly weapon)
    std::string m4_uid = random_string( 10 );
    std::string rope_uid = random_string( 10 );
    m4a1.set_var( "uid", m4_uid );
    rope_map.set_var( "uid", rope_uid );

    GIVEN( "avatar is wearing a backpack with a short rope in it" ) {
        // What happens to the stuff on the ground?
        CAPTURE( here.i_at( ground ).size() );
        // Wear backpack from map and get the new item reference
        std::optional<std::list<item>::iterator> worn = they.wear_item( backpack_map );
        item &backpack = **worn;
        REQUIRE( they.has_item( backpack ) );
        // Put the rope in
        item_location rope = they.i_add( rope_map );
        REQUIRE( they.has_item( *rope ) );

        WHEN( "they pick up the M4" ) {
            // Get item_location for m4 on the map
            item_location m4_loc( map_cursor( they.pos() ), &m4a1 );
            const drop_locations &thing = { std::make_pair( m4_loc, 1 ) };
            CHECK_FALSE( backpack.has_item( m4a1 ) );
            // Now pick up the M4
            they.pick_up( thing );
            process_activity( they );

            // Neither the rope nor the M4 should have been dropped
            THEN( "they should have the rope and M4 still in possession" ) {
                CHECK( character_has_item_with_var_val( they, "uid", rope_uid ) );
                CHECK( character_has_item_with_var_val( they, "uid", m4_uid ) );
            }
        }
    }
}

