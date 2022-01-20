#include "cata_catch.h"
#include "map.h"
#include "map_helpers.h"
#include "avatar.h"
#include "player_helpers.h"
#include "item.h"
#include "item_pocket.h"
#include "pickup.h"

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_backpack_hiking( "backpack_hiking" );
static const itype_id itype_marble( "marble" );
static const itype_id itype_pebble( "pebble" );
static const itype_id itype_codeine( "codeine" );
static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_prescription( "bottle_plastic_pill_prescription" );
static const itype_id itype_plastic_bag( "bag_plastic" );
static const itype_id itype_paper( "paper" );
static const itype_id itype_paper_wrapper( "wrapper" );
static const itype_id itype_chocolate_candy( "candy2" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_light_battery( "light_battery_cell" );

enum Item {
    CODEINE, ASPIRIN, PLASTIC_BAG_CHOCOLATE, BATTERY, FLASHLIGHT
};

// Returns true if the container has an item with the same type and quantity
static bool container_has_item( const item &container, const item &what )
{
    return container.is_container() && container.has_item_with( [&what]( const item & it ) {
        return it.typeId() == what.typeId() && it.charges == what.charges;
    } );
}

// Add the given item to auto-pickup character rules and check rules.
static void add_autopickup_rule( const item *what, bool blacklist = false )
{
    auto_pickup::player_settings &rules = get_auto_pickup();

    rules.add_rule( what );
    REQUIRE( rules.has_rule( what ) );

    std::string item_name = what->tname( 1, false );
    rule_state pickup_state = rules.check_item( item_name );
    REQUIRE( pickup_state == rule_state::WHITELISTED );
}

// Add the given items to auto-pickup character rules and check rules.
static void add_autopickup_rules( const std::list<const item *> what, bool blacklist = false )
{
    for( const item *entry : what ) {
        add_autopickup_rule( entry, blacklist );
    }
}

// Simulate character moving over a tile that contains items.
static void simulate_auto_pickup( const tripoint &pos, avatar &they )
{
    Pickup::autopickup( pos );
    process_activity( they );
}

static void expect_to_find( const item &container, const std::list<const item *> what )
{
    // make sure all items on the list have been picked up.
    REQUIRE( container.all_items_top().size() == what.size() );
    for( const item *entry : what ) {
        REQUIRE( container_has_item( container, *entry ) );
    }
}

// Constructs item with given type that contains a set number of content items.
static item item_with_content( itype_id what, itype_id content, int qty = 1 )
{
    item result = item( what );
    item to_place = item( content, calendar::turn, qty );

    result.force_insert_item( to_place, item_pocket::pocket_type::CONTAINER );
    REQUIRE( container_has_item( result, to_place ) );

    return result;
}

// Constructs item with given list of items contained inside the item pocket.
// Use this function to create a container filled with nested containers.
static item item_with_content( itype_id what, std::list<item> content )
{
    item result = item( what );
    for( item &entry : content ) {
        result.force_insert_item( entry, item_pocket::pocket_type::CONTAINER );
        REQUIRE( container_has_item( result, entry ) );
    }
    return result;
}

// Shorthand method for constructing items.
static item item_with_type( itype_id what, int qty = 1 )
{
    item result = item( what, calendar::turn, qty );
    result.set_var( "uid", random_string( 10 ) );
    return result;
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

    // wear backpack from map and get the new item reference
    item &backpack = **( they.wear_item( item( itype_backpack_hiking ) ) );
    REQUIRE( they.has_item( backpack ) );

    // reset character auto-pickup rules
    auto_pickup::player_settings &rules = get_auto_pickup();
    rules.clear_character_rules();
    rules.check_item( "" );

    GIVEN( "avatar spots items on a tile near him" ) {
        // make sure no items exist on the ground before we add them
        REQUIRE( here.i_at( ground ).size() == 0 );

        // add items to the tile on the ground
        std::vector<item *> items_on_ground{
            &here.add_item( ground, item_with_type( itype_codeine, 20 ) ),
            // plastic prescription bottle > aspirin (12)
            &here.add_item( ground, item_with_content( itype_prescription, itype_aspirin, 12 ) ),
            // plastic bag > paper (4), paper wrapper > chocolate candy
            &here.add_item( ground, item_with_content( itype_plastic_bag, std::list<item> {
                item_with_type( itype_paper, 4 ),
                item_with_content( itype_paper_wrapper, itype_chocolate_candy )
            } ) ),
            &here.add_item( ground, item( itype_light_battery ) ),
            &here.add_item( ground, item_with_content( itype_backpack, std::list<item> {
                item_with_type( itype_flashlight )
            } ) ),
            &here.add_item( ground, item_with_type( itype_marble, 10 ) ),
            &here.add_item( ground, item_with_type( itype_pebble, 15 ) )
        };
        for( item *entry : items_on_ground ) {
            CHECK_FALSE( backpack.has_item( *entry ) );
        }
        WHEN( "they have codeine pills in auto-pickup rules" ) {
            add_autopickup_rule( items_on_ground[Item::CODEINE] );
            THEN( "codeine pills should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const item *> {
                    items_on_ground[Item::CODEINE]
                } );
            }
        }
        WHEN( "they have aspirin pills in auto-pickup rules" ) {
            const item *aspirin = items_on_ground[Item::ASPIRIN];
            const item *codeine = items_on_ground[Item::CODEINE];
            add_autopickup_rules( std::list<const item *> {
                codeine, aspirin
            } );
            THEN( "prescription bottle with aspirin pills should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const item *> {
                    codeine, aspirin
                } );
            }
        }
        WHEN( "they have chocolate candy in auto-pickup rules" ) {
            item item_chocolate_candy = item_with_type( itype_chocolate_candy );
            add_autopickup_rule( &item_chocolate_candy );

            item *item_plastic_bag = items_on_ground[PLASTIC_BAG_CHOCOLATE];
            item *item_chocolate_wrapper = *std::next( item_plastic_bag->all_items_top().begin(), 1 );

            THEN( "paper wrapper with chocolate candy should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const item *> {
                    item_chocolate_wrapper
                } );
            }
        }
        WHEN( "they have light battery in auto-pickup rules" ) {
            item *light_battery = items_on_ground[Item::BATTERY];
            add_autopickup_rule( light_battery );

            item *flashlight = *items_on_ground[Item::FLASHLIGHT]->all_items_top().begin();
            REQUIRE( flashlight->reload( they, item_location( location, light_battery ), 1 ) );

            THEN( "light battery from flashlight should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const item *> {
                    light_battery
                } );
            }
        }
    }
}
