#include "avatar.h"
#include "cata_catch.h"
#include "item.h"
#include "item_pocket.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "pickup.h"

static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_box_small( "box_small" );
static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_bottle_plastic( "bottle_plastic" );
static const itype_id itype_bottle_plastic_pill_prescription( "bottle_plastic_pill_prescription" );
static const itype_id itype_candy2( "candy2" );
static const itype_id itype_can_food( "can_food" );
static const itype_id itype_can_tuna( "can_tuna" );
static const itype_id itype_codeine( "codeine" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_light_battery_cell( "light_battery_cell" );
static const itype_id itype_marble( "marble" );
static const itype_id itype_meat_canned( "meat_canned" );
static const itype_id itype_money_five( "money_five" );
static const itype_id itype_money_one( "money_one" );
static const itype_id itype_money_ten( "money_ten" );
static const itype_id itype_paper( "paper" );
static const itype_id itype_pebble( "pebble" );
static const itype_id itype_wallet_leather( "wallet_leather" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_wrapper( "wrapper" );

static const item_pocket::pocket_type pocket_type_container = item_pocket::pocket_type::CONTAINER;

class unique_item
{
    private:
        item instance;

    public:
        // Create a simple wrapper for given item with random UID.
        explicit unique_item( item &from, bool no_uid = false ) {
            instance = from;
            if( get_uid().empty() && !no_uid ) {
                instance.set_var( "uid", random_string( 10 ) );
            }
        }
        // Construct item with given type and quantity.
        explicit unique_item( itype_id type, int quantity = 1, bool no_uid = false ) {
            instance = item( type, calendar::turn, quantity );
            if( !no_uid ) {
                instance.set_var( "uid", random_string( 10 ) );
            }
        }
        // Construct item with given type and list of items contained inside item pocket.
        unique_item( itype_id type, std::vector<const unique_item *> content ) {
            instance = item( type );
            instance.set_var( "uid", random_string( 10 ) );
            for( const unique_item *entry : content ) {
                instance.force_insert_item( *entry->get(), pocket_type_container );
            }
        }
        // Returns the UID value assigned to this item.
        std::string get_uid() const {
            return instance.get_var( "uid" );
        }
        // Returns a pointer to item instance.
        const item *get() const {
            return &instance;
        }
        // Returns true if both items have the same UID string value.
        bool is_same_item( const item *with ) const {
            std::string item_uid = with->get_var( "uid" );
            std::string this_uid = get_uid();
            // if items don't have UID then compare types and charges
            if( this_uid.empty() && item_uid.empty() ) {
                return instance.typeId() == with->typeId() && instance.charges == with->charges;
            }
            return this_uid == item_uid;
        }
        // Spawns this item in the given location on the map.
        // Returns true if item was spawned and false otherwise.
        bool spawn_item( const tripoint &where ) {
            instance = get_map().add_item( where, instance );
            CHECK_FALSE( get_uid().empty() );
            return !instance.is_null();
        }
        // Returns item instance of this unique item in the given location
        item *find_on_ground( const tripoint &where ) const {
            map_stack stack = get_map().i_at( where );
            item_stack::iterator found = std::find_if( stack.begin(), stack.end(), [this]( item & it ) {
                return is_same_item( &it );
            } );
            return found != stack.end() ? &*found : &null_item_reference();
        }
        // Returns true if this item if found in the given map stack
        bool is_on_ground( const tripoint &where ) const {
            item *found = find_on_ground( where );
            return !found->is_null();
        }
        // Returns a pointer to the item that matches this item in given container or null.
        const item *find_in_container( item &where ) const {
            for( const item *it : where.all_items_top() ) {
                if( is_same_item( it ) ) {
                    return it;
                }
            }
            return &null_item_reference();
        }
};

// Add the given item to auto-pickup character rules and check rules.
static void add_autopickup_rule( const item *what, bool include )
{
    auto_pickup::player_settings &rules = get_auto_pickup();

    rules.add_rule( what, include );
    REQUIRE( rules.has_rule( what ) );

    std::string item_name = what->tname( 1, false );
    rule_state pickup_state = rules.check_item( item_name );
    REQUIRE( pickup_state == ( include ? rule_state::WHITELISTED : rule_state::BLACKLISTED ) );
}

// Add the given items to auto-pickup character rules and check rules.
static void add_autopickup_rules( std::map<const unique_item *, bool> what )
{
    std::map<const unique_item *, bool>::iterator it = what.begin();
    for( it = what.begin(); it != what.end(); it++ ) {
        add_autopickup_rule( it->first->get(), it->second );
    }
}

// Add the given items to auto-pickup character rules and check rules
static void add_autopickup_rules( std::vector<unique_item *> what, bool include )
{
    for( unique_item *it : what ) {
        add_autopickup_rule( it->get(), include );
    }
}

// Simulate character moving over a tile that contains items.
static void simulate_auto_pickup( const tripoint &pos, avatar &they )
{
    Pickup::autopickup( pos );
    process_activity( they );
}

// Require that the given list of items be found contained in item.
static void expect_to_find( const item &in, const std::list<const unique_item *> what )
{
    CHECK_FALSE( in.is_null() );
    CHECK( in.all_items_top().size() == what.size() );

    // make sure all items on the list have been picked up
    for( const unique_item *entry : what ) {
        REQUIRE( in.has_item_with( [entry, in]( const item & it ) {
            return &it == &in || entry->is_same_item( &it );
        } ) );
    }
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
    item &backpack = **( they.wear_item( item( itype_backpack ) ) );
    REQUIRE( they.has_item( backpack ) );

    // reset character auto-pickup rules
    auto_pickup::player_settings &rules = get_auto_pickup();
    rules.clear_character_rules();
    rules.check_item( "" );

    GIVEN( "avatar is about to walk over a tile filled with items" ) {
        // make sure no items exist on the ground before we add them
        REQUIRE( here.i_at( ground ).empty() );

        // add random items to the tile on the ground
        here.add_item( ground, item( itype_marble, calendar::turn, 10 ) );
        here.add_item( ground, item( itype_pebble, calendar::turn, 15 ) );

        // codeine (20)(WL)
        WHEN( "there is an item on the ground whitelisted in auto-pickup rules" ) {
            unique_item item_codeine = unique_item( itype_codeine, 20 );
            REQUIRE( item_codeine.spawn_item( ground ) );
            add_autopickup_rule( item_codeine.get(), true );

            THEN( "the whitelisted item should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_codeine } );
            }
        }
        // plastic prescription bottle > (WL)aspirin (12)
        WHEN( "there is a container on the ground containing only items whitelisted in auto-pickup rules" ) {
            unique_item item_aspirin = unique_item( itype_aspirin, 12 );
            unique_item item_prescription_bottle = unique_item(
            itype_bottle_plastic_pill_prescription, {
                &item_aspirin
            } );
            REQUIRE( item_prescription_bottle.spawn_item( ground ) );
            add_autopickup_rule( item_aspirin.get(), true );

            THEN( "the container along with its contents should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_prescription_bottle } );
                expect_to_find( *item_prescription_bottle.find_in_container( backpack ), {
                    &item_aspirin
                } );
            }
        }
        // plastic bag > paper (5), paper wrapper (WL) > chocolate candy (3)
        WHEN( "there is a container on the ground with a deeply nested item whitelisted in auto-pickup rules" ) {
            const unique_item item_paper = unique_item( itype_paper, 5 );
            const unique_item item_chocolate_candy = unique_item( itype_candy2, 3 );
            const unique_item item_paper_wrapper = unique_item( itype_wrapper, {
                &item_chocolate_candy
            } );
            unique_item item_plastic_bag = unique_item( itype_bag_plastic, {
                &item_paper, &item_paper_wrapper
            } );
            REQUIRE( item_plastic_bag.spawn_item( ground ) );
            add_autopickup_rule( item_chocolate_candy.get(), true );

            THEN( "only the deeply nested item should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_paper_wrapper } );
                expect_to_find( *item_paper_wrapper.find_in_container( backpack ), {
                    &item_chocolate_candy
                } );
                REQUIRE( item_plastic_bag.is_on_ground( ground ) );
            }
        }
        // flashlight > light battery (WL)
        WHEN( "there is a powered tool on the ground loaded with a light battery whitelisted in auto-pickup rules" ) {
            item item_flashlight = item( itype_flashlight );
            item *item_light_battery = &here.add_item( ground, item( itype_light_battery_cell ) );
            REQUIRE_FALSE( item_light_battery->is_null() );

            // insert light battery into flashlight by reloading the item
            item_flashlight.reload( they, item_location( location, item_light_battery ), 1 );
            // battery was removed from ground so update variable
            item_light_battery = item_flashlight.magazine_current();

            unique_item uitem_light_battery = unique_item( *item_light_battery, true );
            unique_item uitem_flashlight = unique_item( item_flashlight );
            REQUIRE( uitem_flashlight.spawn_item( ground ) );

            // we want to remove and pickup the battery from the flashlight
            add_autopickup_rule( item_light_battery, true );

            THEN( "the light battery from the tool should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &uitem_light_battery } );
                REQUIRE( uitem_flashlight.is_on_ground( ground ) );
            }
        }
        // leather wallet (WL) > one dollar bill, five dollar bill (WL), 1ten dollar bill
        WHEN( "there is a non-rigid whitelisted container on the ground with three items, one whitelisted" ) {
            unique_item item_money_one = unique_item( itype_money_one );
            unique_item item_money_five = unique_item( itype_money_five );
            unique_item item_money_ten = unique_item( itype_money_ten );
            unique_item item_leather_wallet = unique_item( itype_wallet_leather, {
                &item_money_one, &item_money_five, &item_money_ten
            } );
            REQUIRE( item_leather_wallet.spawn_item( ground ) );
            add_autopickup_rules( { &item_leather_wallet, &item_money_five }, true );

            THEN( "the container should be picked up and non-whitelisted items should be dropped on the ground" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_leather_wallet } );
                expect_to_find( *item_leather_wallet.find_in_container( backpack ), { &item_money_five } );
                REQUIRE( item_money_one.is_on_ground( ground ) );
            }
        }
        // small cardboard box (WL) > paper, chocolate candy (BL), marble
        WHEN( "there is a rigid whitelisted container on the ground with three items, one blacklisted" ) {
            unique_item item_paper = unique_item( itype_paper );
            unique_item item_chocolate_candy = unique_item( itype_candy2 );
            unique_item item_marble = unique_item( itype_marble );
            unique_item cardboard_box = unique_item( itype_box_small, {
                &item_paper, &item_chocolate_candy, &item_marble
            } );
            REQUIRE( cardboard_box.spawn_item( ground ) );
            add_autopickup_rules( { { &cardboard_box, true }, { &item_chocolate_candy, false } } );
            THEN( "the rigid container should be picked up and blacklisted items should be dropped on the ground" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &cardboard_box } );
                expect_to_find( *cardboard_box.find_in_container( backpack ), {
                    &item_paper, &item_marble
                } );
                REQUIRE( item_chocolate_candy.is_on_ground( ground ) );
            }
        }
        // plastic bottle > clean water (2)(WL)
        WHEN( "there is a rigid blacklisted container on the ground with liquid  in auto-pickup rules" ) {
            // construct and fill bottle with clean water
            item item_plastic_bottle = item( itype_bottle_plastic );
            unique_item item_clean_water = unique_item( itype_water_clean, 2 );
            REQUIRE( item_plastic_bottle.fill_with( *item_clean_water.get() ) == 2 );

            // spawm bottle filled with clean water in the world
            unique_item item_bottled_water = unique_item( item_plastic_bottle );
            REQUIRE( item_bottled_water.spawn_item( ground ) );

            add_autopickup_rules( { { &item_clean_water, true }, { &item_bottled_water, false } } );
            THEN( "the liquid should not be picked up from the container" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, {} );

                // check to see if item has remained on the ground
                REQUIRE( item_bottled_water.is_on_ground( ground ) );
            }
        }
        // small tin can (sealed) > canned tuna fish (WL), canned meat
        WHEN( "there is a sealed container on the ground containing items whitelisted in auto-pickup rules" ) {
            item item_small_tin_can = item( itype_can_food );
            unique_item item_canned_tuna = unique_item( itype_can_tuna, 1, true );
            unique_item item_canned_meat = unique_item( itype_meat_canned, 1, true );

            // insert items inside can and seal it
            item_small_tin_can.force_insert_item( *item_canned_tuna.get(), pocket_type_container );
            item_small_tin_can.force_insert_item( *item_canned_meat.get(), pocket_type_container );
            item_small_tin_can.seal();

            unique_item item_sealed_tuna = unique_item( item_small_tin_can );
            REQUIRE( item_sealed_tuna.spawn_item( ground ) );

            add_autopickup_rule( item_canned_tuna.get(), true );
            THEN( "the container should be picked up instead of whitelisted items" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_sealed_tuna } );

                // make sure the item seal was not broken
                REQUIRE( item_sealed_tuna.find_in_container( backpack )->all_pockets_sealed() );
            }
        }
    }
}
