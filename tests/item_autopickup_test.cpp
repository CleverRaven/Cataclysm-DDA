#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "pickup.h"
#include "item_pocket.h"

static const flag_id flag_FROZEN = flag_id( "FROZEN" );

static const itype_id itype_backpack( "backpack_hiking" );

static const itype_id itype_marble( "marble" );
static const itype_id itype_pebble( "pebble" );

static const itype_id itype_codeine( "codeine" );
static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_pbottle( "bottle_plastic_pill_prescription" );

enum Item {
    CODEINE, PRESCRIPTION_BOTTLE
};

// Returns true if the container has an item with the same type and quantity
static bool container_has_item( const item &container, const item &what )
{
    return container.is_container() && container.has_item_with( [&what]( const item & it ) {
        return it.typeId() == what.typeId() && it.charges == what.charges;
    } );
}

// Remove FROZEN flag from item to allow for easy rule matching.
// When frozen items will have '(frozen)' in their name making matching more difficult.
static void unfreeze_item( item *which )
{
    which->unset_flag( flag_FROZEN );
    REQUIRE_FALSE( which->has_flag( flag_FROZEN ) );
}

// Add the given items to auto-pickup character rules and check rules.
static void add_autopickup_rules( const std::list<item *> what )
{
    auto_pickup::player_settings &rules = get_auto_pickup();
    for( item *entry : what ) {
        rules.add_rule( entry );
        REQUIRE( rules.has_rule( entry ) );
        REQUIRE( rules.check_item( entry->tname( 1, false ) ) == rule_state::WHITELISTED );
    }
}

// Simulate character moving over a tile that contains items.
static void simulate_auto_pickup( const tripoint &pos, avatar &they )
{
    Pickup::pick_up( pos, -1 );
    process_activity( they );
}

static void expect_to_find( const item &container, const std::list<item *> what )
{
    // make sure all items on the list have been picked up.
    REQUIRE( container.all_items_top().size() == what.size() );
    for( item *entry : what ) {
        REQUIRE( container_has_item( container, *entry ) );
    }
}

// Constructs item with given type that contains a set number of content items.
static item item_with_content( itype_id what, itype_id content, int qty )
{
    item result = item( what );
    item to_place = item( content, calendar::turn, qty );

    result.force_insert_item( to_place, item_pocket::pocket_type::CONTAINER );
    REQUIRE( container_has_item( result, to_place ) );

    return result;
}

// Shorthand method to construct items with specific quantity.
static item item_with_qty( itype_id what, int qty )
{
    return item( what, calendar::turn, qty );
}

// Test if autopickup feature works
TEST_CASE( "items can be auto-picked up from the ground", "[pickup][item]" )
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

    // wear backpack from map and get the new item reference
    they.worn.clear();
    item &backpack = **( they.wear_item( backpack_item ) );
    REQUIRE( they.has_item( backpack ) );

    // reset character auto-pickup rules
    get_auto_pickup().clear_character_rules();

    GIVEN( "avatar spots items on a tile near him" ) {
        // add items to the tile on the ground
        std::vector<item *> items_on_ground{
            &here.add_item( ground, item_with_qty( itype_codeine, 20 ) ),
            &here.add_item( ground, item_with_content( itype_pbottle, itype_aspirin, 12 ) ),
            &here.add_item( ground, item_with_qty( itype_marble, 10 ) ),
            &here.add_item( ground, item_with_qty( itype_pebble, 15 ) )
        };
        for( item *entry : items_on_ground ) {
            unfreeze_item( entry );
            CHECK_FALSE( backpack.has_item( *entry ) );
        }
        WHEN( "they have codeine pills in auto-pickup rules" ) {
            add_autopickup_rules( std::list<item *> {
                items_on_ground[Item::CODEINE]
            } );
            THEN( "codeine pills should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<item *> {
                    items_on_ground[Item::CODEINE]
                } );
            }
        }
    }
}
