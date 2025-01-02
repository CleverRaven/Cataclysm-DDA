#include "cata_catch.h"
#include "npc.h"
#include "npc_class.h"

static npc_template_id const npc_template_test_npc_trader( "test_npc_trader" );

TEST_CASE( "npc_shop_cons_rates", "[npc][trade]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_trader );

    shopkeeper_cons_rates const &myrates = guy.myclass->get_shopkeeper_cons_rates();

    WHEN( "item has no matches" ) {
        REQUIRE( myrates.get_rate( item( "hammer" ), guy ) == myrates.default_rate );
    }
    WHEN( "item is matched by junk threshold" ) {
        item const glass_shard( "glass_shard" );
        REQUIRE( glass_shard.price_no_contents( true ) < units::to_cent( myrates.junk_threshold ) );
        REQUIRE( myrates.get_rate( glass_shard, guy ) == -1 );
    }
    WHEN( "item is matched by typeid" ) {
        REQUIRE( myrates.get_rate( item( "bow_saw" ), guy ) == 2 );
    }
    WHEN( "item is matched by typeid and condition is true" ) {
        guy.set_value( "bool_dinner_bow_saw_eater", "yes" );
        REQUIRE( myrates.get_rate( item( "bow_saw" ), guy ) == 99 );
    }
    WHEN( "item is matched by category" ) {
        REQUIRE( myrates.get_rate( item( "coin_gold" ), guy ) == 10 );
    }
    WHEN( "item is matched by category but it's junk" ) {
        REQUIRE( myrates.get_rate( item( "coin_dollar" ), guy ) == -1 );
    }
    WHEN( "item is matched by item_group" ) {
        REQUIRE( myrates.get_rate( item( "test_pipe" ), guy ) == 100 );
    }
    WHEN( "item is matched by entry with both item_group and category" ) {
        REQUIRE( myrates.get_rate( item( "test_nuclear_carafe" ), guy ) == 50 );
    }
    WHEN( "item is matched by category and an overriding entry" ) {
        REQUIRE( myrates.get_rate( item( "FMCNote" ), guy ) == 25 );
    }
}
