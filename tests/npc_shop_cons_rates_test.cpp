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
        item hammer( "hammer" );
        CHECK( myrates.get_rate( { map_cursor{ tripoint_bub_ms{} }, &hammer }, guy ) ==
               myrates.default_rate );
    }
    WHEN( "item is matched by junk threshold" ) {
        item glass_shard( "glass_shard" );
        REQUIRE( glass_shard.price_no_contents( true ) < units::to_cent( myrates.junk_threshold ) );
        CHECK( myrates.get_rate( {map_cursor{ tripoint_bub_ms{} }, &glass_shard}, guy ) == -1 );
    }
    item bow_saw( "bow_saw" );
    item_location saw_loc( map_cursor{ tripoint_bub_ms{} }, &bow_saw );
    WHEN( "item is matched by typeid" ) {
        CHECK( myrates.get_rate( saw_loc, guy ) == 2 );
    }
    WHEN( "item is matched by typeid and condition is true" ) {
        guy.set_value( "bool_dinner_bow_saw_eater", "yes" );
        CHECK( myrates.get_rate( saw_loc, guy ) == 99 );
    }
    WHEN( "item is matched by category" ) {
        item gold( "coin_gold" );
        REQUIRE( myrates.get_rate( {map_cursor{ tripoint_bub_ms{} }, &gold}, guy ) == 10 );
    }
    WHEN( "item is matched by category but it's junk" ) {
        item dollarydoo( "coin_dollar" );
        REQUIRE( myrates.get_rate( {map_cursor{ tripoint_bub_ms{} }, &dollarydoo}, guy ) == -1 );
    }
    WHEN( "item is matched by item_group" ) {
        item pipe( "test_pipe" );
        CHECK( myrates.get_rate( {map_cursor{ tripoint_bub_ms{} }, &pipe}, guy ) == 100 );
    }
    WHEN( "item is matched by entry with both item_group and category" ) {
        item carafe( "test_nuclear_carafe" );
        CHECK( myrates.get_rate( {map_cursor{ tripoint_bub_ms{} }, &carafe}, guy ) == 50 );
    }
    WHEN( "item is matched by category and an overriding entry" ) {
        item fmcnote( "FMCNote" );
        CHECK( myrates.get_rate( {map_cursor{ tripoint_bub_ms{} }, &fmcnote}, guy ) == 25 );
    }
}
