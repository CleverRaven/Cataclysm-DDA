#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "pickup.h"

static const itype_id itype_backpack( "backpack_hiking" );
static const flag_id flag_FROZEN = flag_id( "FROZEN" );

// Return true if the container has an item of the given type inside
static bool container_has_item_type( const item &container, const itype_id it_type )
{
    return container.is_container() && container.has_item_with( [&it_type]( const item & it ) {
        return it.typeId() == it_type;
    } );
}

// Test if autopickup feature works for single non-container items
TEST_CASE( "Autopickup single item", "[pickup][item]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_avatar();
    clear_map();

    // this is where items will be picked up from
    const tripoint ground = they.pos();
    const map_cursor location = map_cursor( ground );

    // spawn container where items will go when picked up
    item &backpack_item = here.add_item( ground, item( itype_backpack ) );

    // items to be spawned and autopicked up by player
    std::list<item *> autopickup_items{
        &here.add_item( ground, item( "aspirin" ) ),
    };
    for( item *entry : autopickup_items ) {
        // make sure item is not frozen in the world
        entry->unset_flag( flag_FROZEN );
        REQUIRE_FALSE( entry->has_flag( flag_FROZEN ) );

        // make sure backpack can contain item
        REQUIRE( backpack_item.can_contain( *entry ).success() );
    }
    GIVEN( "avatar set the pickup rules and is wearing a backpack" ) {
        auto_pickup::player_settings auto_pickup = get_auto_pickup();

        // add and validate autopickup rules
        for( item *entry : autopickup_items ) {
            auto_pickup.add_rule( entry );
            REQUIRE( auto_pickup.has_rule( entry ) );
            REQUIRE( auto_pickup.check_item( entry->tname( 1, false ) ) == rule_state::WHITELISTED );
        }
        // Wear backpack from map and get the new item reference
        they.worn.clear();
        item &backpack = **( they.wear_item( backpack_item ) );
        REQUIRE( they.has_item( backpack ) );

        WHEN( "they pickup the items on ground" ) {
            // what happens to the stuff on the ground?
            CAPTURE( here.i_at( ground ).size() );

            // ensure container doesn't already contain items
            for( item *entry : autopickup_items ) {
                CHECK_FALSE( backpack.has_item( *entry ) );
            }
            // simulate walking over the tile filled with items
            Pickup::pick_up( location.pos(), -1 );
            process_activity( they );

            THEN( "items matching autopickup rules should be in the backpack" ) {
                for( item *entry : autopickup_items ) {
                    CHECK( container_has_item_type( backpack, entry->typeId() ) );
                }
            }
        }
    }
}
