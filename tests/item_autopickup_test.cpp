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
static const itype_id itype_bottle_plastic_pill_prescription( "bottle_plastic_pill_prescription" );
static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_paper( "paper" );
static const itype_id itype_wrapper( "wrapper" );
static const itype_id itype_candy2( "candy2" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_light_battery_cell( "light_battery_cell" );

class unique_item
{
    private:
        item instance;

    public:
        // Construct item with given type and quantity.
        unique_item( itype_id type, int quantity = 1 ) {
            instance = item( type, calendar::turn, quantity );
            instance.set_var( "uid", random_string( 10 ) );
        }
        // Construct item with given type and list of items contained inside item pocket.
        unique_item( itype_id type, std::vector<const unique_item *> content ) {
            instance = item( type );
            instance.set_var( "uid", random_string( 10 ) );
            for( const unique_item *entry : content ) {
                insert_item( entry->get() );
            }
        }
        // Create a simple wrapper for given item with random UID.
        unique_item( item &from, bool no_uid = false ) {
            instance = from;
            if( get_uid().empty() && !no_uid ) {
                instance.set_var( "uid", random_string( 10 ) );
            }
        }
        // Returns the UID value assigned to this item.
        const std::string get_uid() const {
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
        // Force insert the given item to container pocket of this item.
        void insert_item( const item *what ) {
            instance.force_insert_item( *what, item_pocket::pocket_type::CONTAINER );
        }
        // Spawns this item in the given location on the map.
        // Returns true if item was spawned and false otherwise.
        bool spawn_item( const tripoint &where ) {
            instance = get_map().add_item( where, instance );
            CHECK_FALSE( get_uid().empty() );
            return &instance != &null_item_reference();
        }
};

// Add the given item to auto-pickup character rules and check rules.
static void add_autopickup_rule( const item *what, bool exclude )
{
    auto_pickup::player_settings &rules = get_auto_pickup();

    rules.add_rule( what );
    REQUIRE( rules.has_rule( what ) );

    std::string item_name = what->tname( 1, false );
    rule_state pickup_state = rules.check_item( item_name );
    REQUIRE( pickup_state == rule_state::WHITELISTED );
}

// Add the given items to auto-pickup character rules and check rules.
static void add_autopickup_rules( const std::list<const unique_item *> what, bool exclude )
{
    for( const unique_item *entry : what ) {
        add_autopickup_rule( entry->get(), exclude );
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
    // make sure all items on the list have been picked up.
    REQUIRE( in.all_items_top().size() == what.size() );
    for( const unique_item *entry : what ) {
        REQUIRE( in.has_item_with( [entry]( const item & it ) {
            return entry->is_same_item( &it );
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
    item &backpack = **( they.wear_item( item( itype_backpack_hiking ) ) );
    REQUIRE( they.has_item( backpack ) );

    // reset character auto-pickup rules
    auto_pickup::player_settings &rules = get_auto_pickup();
    rules.clear_character_rules();
    rules.check_item( "" );

    GIVEN( "avatar spots items on a tile near him" ) {
        // make sure no items exist on the ground before we add them
        REQUIRE( here.i_at( ground ).size() == 0 );

        // add random items to the tile on the ground
        here.add_item( ground, item( itype_marble, calendar::turn, 10 ) );
        here.add_item( ground, item( itype_pebble, calendar::turn, 15 ) );

        // codeine (20)
        WHEN( "they have codeine pills in auto-pickup rules" ) {
            unique_item item_codeine = unique_item( itype_codeine, 20 );
            REQUIRE( item_codeine.spawn_item( ground ) );
            add_autopickup_rule( item_codeine.get(), false );

            THEN( "codeine pills should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const unique_item *> {
                    &item_codeine
                } );
            }
        }
        // plastic prescription bottle > aspirin (12)
        WHEN( "they have aspirin pills in auto-pickup rules" ) {
            unique_item item_aspirin = unique_item( itype_aspirin, 12 );
            REQUIRE( item_aspirin.spawn_item( ground ) );
            add_autopickup_rule( item_aspirin.get(), false );

            THEN( "prescription bottle with aspirin pills should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const unique_item *> {
                    &item_aspirin
                } );
            }
        }
        // plastic bag > paper (5), paper wrapper > chocolate candy (3)
        WHEN( "they have chocolate candy in auto-pickup rules" ) {
            const unique_item item_paper = unique_item( itype_paper, 5 );
            const unique_item item_chocolate_candy = unique_item( itype_candy2, 3 );
            const unique_item item_paper_wrapper = unique_item( itype_wrapper,
            std::vector<const unique_item *> {
                &item_chocolate_candy
            } );
            unique_item item_plastic_bag = unique_item( itype_bag_plastic,
            std::vector<const unique_item *> {
                &item_paper, &item_paper_wrapper
            } );
            REQUIRE( item_plastic_bag.spawn_item( ground ) );
            add_autopickup_rule( item_chocolate_candy.get(), false );

            THEN( "paper wrapper with chocolate candy should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const unique_item *> {
                    &item_paper_wrapper
                } );
            }
        }
        // flashlight > light battery
        WHEN( "they have light battery in auto-pickup rules" ) {
            item *item_light_battery = &here.add_item( ground, item( itype_light_battery_cell ) );
            REQUIRE( item_light_battery != &null_item_reference() );

            item &item_flashlight = here.add_item( ground, item( itype_flashlight ) );
            REQUIRE( &item_flashlight != &null_item_reference() );

            // insert light battery into flashlight by reloading the item
            item_flashlight.reload( they, item_location( location, item_light_battery ), 1 );
            // battery was removed from ground so update variable
            item_light_battery = item_flashlight.magazine_current();

            unique_item uitem_light_battery = unique_item( *item_light_battery, true );
            unique_item uitem_flashlight = unique_item( item_flashlight );

            // we want to remove and pickup the battery from the flashlight
            add_autopickup_rule( item_light_battery, false );

            THEN( "light battery from flashlight should be picked up" ) {
                simulate_auto_pickup( ground, they );
                expect_to_find( backpack, std::list<const unique_item *> {
                    &uitem_light_battery
                } );
            }
        }
    }
}
