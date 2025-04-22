#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "auto_pickup.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "enums.h"
#include "item.h"
#include "item_location.h"
#include "item_stack.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "options.h"
#include "pickup.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "rng.h"
#include "type_id.h"

static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bag_body_bag( "bag_body_bag" );
static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_bottle_plastic( "bottle_plastic" );
static const itype_id itype_bottle_plastic_pill_prescription( "bottle_plastic_pill_prescription" );
static const itype_id itype_box_cigarette( "box_cigarette" );
static const itype_id itype_box_small( "box_small" );
static const itype_id itype_can_medium( "can_medium" );
static const itype_id itype_can_tuna( "can_tuna" );
static const itype_id itype_candy2( "candy2" );
static const itype_id itype_candycigarette( "candycigarette" );
static const itype_id itype_cig( "cig" );
static const itype_id itype_codeine( "codeine" );
static const itype_id itype_corpse( "corpse" );
static const itype_id itype_diving_flashlight_small_hipower( "diving_flashlight_small_hipower" );
static const itype_id itype_light_battery_cell( "light_battery_cell" );
static const itype_id itype_marble( "marble" );
static const itype_id itype_meat_canned( "meat_canned" );
static const itype_id itype_money_five( "money_five" );
static const itype_id itype_money_one( "money_one" );
static const itype_id itype_money_ten( "money_ten" );
static const itype_id itype_paper( "paper" );
static const itype_id itype_pebble( "pebble" );
static const itype_id itype_rolling_paper( "rolling_paper" );
static const itype_id itype_steel_lump( "steel_lump" );
static const itype_id itype_storage_battery( "storage_battery" );
static const itype_id itype_wallet_leather( "wallet_leather" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_wrapper( "wrapper" );

static const pocket_type pocket_type_container = pocket_type::CONTAINER;

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
        unique_item( itype_id type, const std::vector<const unique_item *> &content ) {
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
        // Set the owner of this item instance to given avatar.
        void set_owner( avatar &who ) {
            instance.set_owner( who );
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
        bool spawn_item( const tripoint_bub_ms &where ) {
            instance = get_map().add_item( where, instance );
            CHECK_FALSE( get_uid().empty() );
            return !instance.is_null();
        }
        // Returns item instance of this unique item in the given location
        item *find_on_ground( const tripoint_bub_ms &where ) const {
            map_stack stack = get_map().i_at( where );
            item_stack::iterator found = std::find_if( stack.begin(), stack.end(), [this]( item & it ) {
                return is_same_item( &it );
            } );
            return found != stack.end() ? &*found : &null_item_reference();
        }
        // Returns true if this item if found in the given map stack
        bool is_on_ground( const tripoint_bub_ms &where ) const {
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
static void add_autopickup_rules( const std::vector<unique_item *> &what, bool include )
{
    for( unique_item *it : what ) {
        add_autopickup_rule( it->get(), include );
    }
}

// Simulate character moving over a tile that contains items.
static void simulate_auto_pickup( const tripoint_bub_ms &pos, avatar &they )
{
    Pickup::autopickup( pos );
    process_activity( they );
}

// Require that the given list of items be found contained in item.
static void expect_to_find( const item &in, const std::list<const unique_item *> &what )
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

// Reset all variables and settings needed to do a clean test.
static void clear_everything()
{
    clear_avatar();
    clear_map();

    auto_pickup::player_settings &rules = get_auto_pickup();
    rules.clear_character_rules();
    // this will clear the cache and recreate the map
    rules.check_item( "" );

    // reset all autopickup options to default values
    options_manager &options = get_options();
    options.get_option( "AUTO_PICKUP_WEIGHT_LIMIT" ).setValue( "0" );
    options.get_option( "AUTO_PICKUP_VOLUME_LIMIT" ).setValue( "0" );
    options.get_option( "AUTO_PICKUP_OWNED" ).setValue( "false" );
}

TEST_CASE( "auto_pickup_should_recognize_container_content", "[autopickup][item]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_everything();

    // this is where items will be picked up from
    const tripoint_bub_ms ground = they.pos_bub();
    const map_cursor location = map_cursor( ground );

    // wear backpack and store item reference
    auto backpack_iter = *they.wear_item( item( itype_backpack ) );
    item &backpack = *backpack_iter;
    REQUIRE( they.has_item( backpack ) );

    GIVEN( "avatar is about to walk over a tile filled with items" ) {
        // make sure no items exist on the ground before we add them
        REQUIRE( here.i_at( ground ).empty() );

        // add random items to the tile on the ground
        here.add_item( ground, item( itype_marble, calendar::turn ) );
        here.add_item( ground, item( itype_pebble, calendar::turn ) );

        // codeine (WL)
        WHEN( "there is an item on the ground whitelisted in auto-pickup rules" ) {
            unique_item item_codeine = unique_item( itype_codeine );
            REQUIRE( item_codeine.spawn_item( ground ) );
            add_autopickup_rule( item_codeine.get(), true );

            THEN( "the whitelisted item should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_codeine } );
            }
        }
        // plastic prescription bottle > (WL)aspirin
        WHEN( "there is a container on the ground containing only items whitelisted in auto-pickup rules" ) {
            unique_item item_aspirin = unique_item( itype_aspirin );
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
        // plastic bag > paper, paper wrapper (WL) > chocolate candy
        WHEN( "there is a container on the ground with a deeply nested item whitelisted in auto-pickup rules" ) {
            const unique_item item_paper = unique_item( itype_paper );
            const unique_item item_chocolate_candy = unique_item( itype_candy2 );
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
                // make sure the item has remained on the ground
                REQUIRE( item_plastic_bag.is_on_ground( ground ) );
            }
        }
    }
}

TEST_CASE( "auto_pickup_should_improve_your_life", "[autopickup][item]" )
{
    avatar &they = get_avatar();
    map &here = get_map();
    clear_everything();

    // this is where items will be picked up from
    const tripoint_bub_ms ground = they.pos_bub();
    const map_cursor location = map_cursor( ground );

    // wear backpack and store item reference
    auto backpack_iter = *they.wear_item( item( itype_backpack ) );
    item &backpack = *backpack_iter;
    REQUIRE( they.has_item( backpack ) );

    // flashlight > light battery (WL)
    WHEN( "there is a powered tool on the ground loaded with a light battery whitelisted in auto-pickup rules" ) {
        item item_flashlight = item( itype_diving_flashlight_small_hipower );
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
            // make sure the item has remained on the ground
            REQUIRE( uitem_flashlight.is_on_ground( ground ) );
        }
    }
}

TEST_CASE( "auto_pickup_should_consider_item_rigidness_and_seal", "[autopickup][item]" )
{
    avatar &they = get_avatar();
    clear_everything();

    // this is where items will be picked up from
    const tripoint_bub_ms ground = they.pos_bub();
    const map_cursor location = map_cursor( ground );

    // wear backpack and store item reference
    auto backpack_iter = *they.wear_item( item( itype_backpack ) );
    item &backpack = *backpack_iter;
    REQUIRE( they.has_item( backpack ) );

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
            // make sure the item has remained on the ground
            REQUIRE( item_money_one.is_on_ground( ground ) );
        }
    }
    // small cardboard box (WL) > paper, chocolate candy (BL), marble
    WHEN( "there is a rigid whitelisted container on the ground with three items (one blacklisted)" ) {
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
            // make sure the item has remained on the ground
            REQUIRE( item_chocolate_candy.is_on_ground( ground ) );
        }
    }
    // plastic bottle > clean water (2)(WL)
    WHEN( "there is a rigid blacklisted container on the ground with liquid whitelisted in auto-pickup rules" ) {
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
            // make sure the item has remained on the ground
            REQUIRE( item_bottled_water.is_on_ground( ground ) );
        }
    }
    // small tin can (sealed) > canned tuna fish (WL), canned meat
    WHEN( "there is a sealed container on the ground containing items whitelisted in auto-pickup rules" ) {
        item item_medium_tin_can = item( itype_can_medium );
        unique_item item_canned_tuna = unique_item( itype_can_tuna, true );
        unique_item item_canned_meat = unique_item( itype_meat_canned, true );

        // insert items inside can and seal it
        item_medium_tin_can.force_insert_item( *item_canned_tuna.get(), pocket_type_container );
        item_medium_tin_can.force_insert_item( *item_canned_meat.get(), pocket_type_container );
        item_medium_tin_can.seal();

        unique_item item_sealed_tuna = unique_item( item_medium_tin_can );
        REQUIRE( item_sealed_tuna.spawn_item( ground ) );

        add_autopickup_rule( item_canned_tuna.get(), true );
        THEN( "the container should be picked up instead of whitelisted items" ) {
            simulate_auto_pickup( ground, they );
            expect_to_find( backpack, { &item_sealed_tuna } );
            // make sure the item seal was not broken
            REQUIRE( item_sealed_tuna.find_in_container( backpack )->all_pockets_sealed() );
        }
    }
    // small tin can (sealed) > canned tuna fish (WL), canned meat
    WHEN( "there is a sealed container on the ground containing no whitelisted items" ) {
        item item_medium_tin_can = item( itype_can_medium );
        item item_bottle_plastic = item( itype_bottle_plastic );
        unique_item item_canned_tuna = unique_item( itype_can_tuna, true );
        unique_item item_canned_meat = unique_item( itype_meat_canned, true );

        // insert items inside can and seal it
        item_medium_tin_can.force_insert_item( *item_canned_tuna.get(), pocket_type_container );
        item_medium_tin_can.force_insert_item( *item_canned_meat.get(), pocket_type_container );
        item_medium_tin_can.seal();

        unique_item item_sealed_tuna = unique_item( item_medium_tin_can );
        REQUIRE( item_sealed_tuna.spawn_item( ground ) );
        // autopickup something other than the sealed items
        unique_item item_red_herring = unique_item( item_bottle_plastic );
        add_autopickup_rule( item_red_herring.get(), true );
        THEN( "nothing should be picked up" ) {
            simulate_auto_pickup( ground, they );
            expect_to_find( backpack, {} );
            // make sure the item seal was not broken
            REQUIRE( item_sealed_tuna.is_on_ground( ground ) );
            REQUIRE( item_sealed_tuna.find_on_ground( ground )->all_pockets_sealed() );
        }
    }
}

TEST_CASE( "auto_pickup_should_respect_volume_and_weight_limits", "[autopickup][item]" )
{
    avatar &they = get_avatar();
    clear_everything();

    // this is where items will be picked up from
    const tripoint_bub_ms ground = they.pos_bub();
    const map_cursor location = map_cursor( ground );

    // wear backpack and store item reference
    auto backpack_iter = *they.wear_item( item( itype_backpack ) );
    item &backpack = *backpack_iter;
    REQUIRE( they.has_item( backpack ) );

    // backpack > lump of steel (WL), cigrarette (WL), paper (WL)
    GIVEN( "there is a container with some items that exceed volume or weight limit" ) {
        options_manager &options = get_options();
        options_manager::cOpt &ap_weight_limit = options.get_option( "AUTO_PICKUP_WEIGHT_LIMIT" );
        options_manager::cOpt &ap_volume_limit = options.get_option( "AUTO_PICKUP_VOLUME_LIMIT" );

        // Note: This relies on charge-spawning behavior for steel lumps
        unique_item item_lump_of_steel = unique_item( itype_steel_lump, 5 );
        unique_item item_cigarette = unique_item( itype_cig );
        unique_item item_paper = unique_item( itype_paper );
        unique_item item_backpack = unique_item( itype_backpack, {
            &item_lump_of_steel, &item_cigarette, &item_paper
        } );
        REQUIRE( item_backpack.spawn_item( ground ) );
        add_autopickup_rules( { &item_lump_of_steel, &item_cigarette, &item_paper }, true );
        WHEN( "auto pickup limit game options are disabled" ) {
            ap_weight_limit.setValue( "0" );
            ap_volume_limit.setValue( "0" );
            THEN( "all items should be picked up regardless of weight and volume" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_backpack } );
                expect_to_find( *item_backpack.find_in_container( backpack ), {
                    &item_lump_of_steel, &item_cigarette, &item_paper
                } );
            }
        }
        WHEN( "auto pickup limit game options are not disabled" ) {
            ap_weight_limit.setValue( "80" ); // 4.0 kilograms
            REQUIRE( get_option<int>( "AUTO_PICKUP_WEIGHT_LIMIT" ) == 80 );

            ap_volume_limit.setValue( "20" ); // 1.0 liter
            REQUIRE( get_option<int>( "AUTO_PICKUP_VOLUME_LIMIT" ) == 20 );
            THEN( "only items that do not exceed volume and weight limit should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_cigarette, &item_paper } );
                expect_to_find( *item_backpack.find_on_ground( ground ), { &item_lump_of_steel } );
                // make sure excluded items were not dropped on the ground
                REQUIRE_FALSE( item_lump_of_steel.is_on_ground( ground ) );
            }
        }
    }

    GIVEN( "a whitelist rule for picking up battery charges" ) {
        item item_store_bat( itype_storage_battery );
        item item_bat( itype_battery, calendar::turn, 10 );
        item_store_bat.ammo_set( itype_battery, 10 );
        REQUIRE( !item_store_bat.all_items_top().empty() );
        REQUIRE( item_store_bat.all_items_top().front()->charges == 10 );
        unique_item item_storage_battery = unique_item( item_store_bat );
        unique_item item_battery = unique_item( item_bat );
        REQUIRE( item_storage_battery.spawn_item( ground ) );
        add_autopickup_rules( { &item_storage_battery }, false );
        add_autopickup_rules( { &item_battery }, true );
        THEN( "ignore battery charges" ) {
            simulate_auto_pickup( ground, they );
            expect_to_find( backpack, {} );
            CHECK( item_storage_battery.is_on_ground( ground ) );
            REQUIRE( !item_storage_battery.find_on_ground( ground )->all_items_top().empty() );
            REQUIRE( item_storage_battery.find_on_ground( ground )->all_items_top().front()->charges ==
                     10 );
        }
    }
}

TEST_CASE( "auto_pickup_should_consider_item_ownership", "[autopickup][item]" )
{
    avatar &they = get_avatar();
    clear_everything();

    // this is where items will be picked up from
    const tripoint_bub_ms ground = they.pos_bub();
    const map_cursor location = map_cursor( ground );

    // wear backpack and store item reference
    auto backpack_iter = *they.wear_item( item( itype_backpack ) );
    item &backpack = *backpack_iter;
    REQUIRE( they.has_item( backpack ) );

    // candy cigarette(WL)
    // pack(WL) > cigarette (WL), rolling paper (WL)
    GIVEN( "there is a container with some items and an item outside it on the ground" ) {
        options_manager::cOpt &autopickup_owned = get_options().get_option( "AUTO_PICKUP_OWNED" );
        // make sure the autopickup owned option is disabled
        if( autopickup_owned.value_as<bool>() ) {
            autopickup_owned.setValue( "false" );
        }
        unique_item item_candy_cigarette = unique_item( itype_candycigarette );
        unique_item item_cigarette = unique_item( itype_cig );
        unique_item item_rolling_paper = unique_item( itype_rolling_paper );
        unique_item item_pack = unique_item( itype_box_cigarette, {
            &item_cigarette, &item_rolling_paper
        } );
        WHEN( "only the item outside the container is owned by avatar" ) {
            item_candy_cigarette.set_owner( they );
            REQUIRE( item_candy_cigarette.get()->is_owned_by( they ) );

            REQUIRE( item_candy_cigarette.spawn_item( ground ) );
            REQUIRE( item_pack.spawn_item( ground ) );

            add_autopickup_rules( { &item_candy_cigarette, &item_cigarette, &item_rolling_paper }, true );
            THEN( "only the container should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_pack } );
                expect_to_find( *item_pack.find_in_container( backpack ), { &item_cigarette, &item_rolling_paper } );
                // make sure the item has remained on the ground
                REQUIRE( item_candy_cigarette.is_on_ground( ground ) );
            }
        }
        WHEN( "only the container is owned by avatar" ) {
            item_pack.set_owner( they );
            REQUIRE( item_pack.get()->is_owned_by( they ) );

            REQUIRE( item_candy_cigarette.spawn_item( ground ) );
            REQUIRE( item_pack.spawn_item( ground ) );

            add_autopickup_rules( { &item_candy_cigarette, &item_pack }, true );
            THEN( "the item outside the container should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, { &item_candy_cigarette } );
                expect_to_find( *item_pack.find_on_ground( ground ), { &item_cigarette, &item_rolling_paper } );
            }
        }
    }
}

TEST_CASE( "auto_pickup_should_not_implicitly_pickup_corpses", "[autopickup][item]" )
{
    avatar &they = get_avatar();
    clear_everything();

    // this is where items will be picked up from
    const tripoint_bub_ms ground = they.pos_bub();
    const map_cursor location = map_cursor( ground );

    // wield body bag and store item reference
    they.set_wielded_item( item( itype_bag_body_bag ) );
    item_location body_bag = they.get_wielded_item();
    REQUIRE_FALSE( !body_bag );

    GIVEN( "there is a generic corpse on the ground" ) {
        unique_item item_corpse = unique_item( itype_corpse );
        REQUIRE( item_corpse.spawn_item( ground ) );

        WHEN( "the corpse is whitelisted in auto-pickup rules" ) {
            add_autopickup_rule( item_corpse.get(), true );
            THEN( "the corpse should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( *body_bag, { &item_corpse } );
            }
        }
        WHEN( "the corpse is NOT whitelisted in auto-pickup rules" ) {
            THEN( "the corpse should NOT be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( *body_bag, {} );
            }
            WHEN( "the corpse contains whitelisted items" ) {
                unique_item item_cigarette = unique_item( itype_cig );
                unique_item item_rolling_paper = unique_item( itype_rolling_paper );

                item *found = item_corpse.find_on_ground( ground );
                found->force_insert_item( *item_cigarette.get(), pocket_type_container );
                found->force_insert_item( *item_rolling_paper.get(), pocket_type_container );

                add_autopickup_rules( { &item_cigarette, &item_rolling_paper }, true );
                THEN( "whitelisted items should be picked up without the corpse" ) {
                    simulate_auto_pickup( ground, they );
                    expect_to_find( *body_bag, { &item_cigarette, &item_rolling_paper } );
                }
            }
        }
    }
}
