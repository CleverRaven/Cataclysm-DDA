#include <string>

#include "cata_catch.h"
#include "item.h"
#include "npc.h"
#include "npc_class.h"
#include "shop_cons_rate.h"
#include "type_id.h"
#include "units.h"

static const itype_id itype_FMCNote( "FMCNote" );
static const itype_id itype_bow_saw( "bow_saw" );
static const itype_id itype_coin_dollar( "coin_dollar" );
static const itype_id itype_coin_gold( "coin_gold" );
static const itype_id itype_glass_shard( "glass_shard" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_test_nuclear_carafe( "test_nuclear_carafe" );
static const itype_id itype_test_pipe( "test_pipe" );

static npc_template_id const npc_template_test_npc_trader( "test_npc_trader" );

TEST_CASE( "npc_shop_cons_rates", "[npc][trade]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_trader );

    shopkeeper_cons_rates const &myrates = guy.myclass->get_shopkeeper_cons_rates();

    WHEN( "item has no matches" ) {
        REQUIRE( myrates.get_rate( item( itype_hammer ), guy ) == myrates.default_rate );
    }
    WHEN( "item is matched by junk threshold" ) {
        item const glass_shard( itype_glass_shard );
        REQUIRE( glass_shard.price_no_contents( true ) < units::to_cent( myrates.junk_threshold ) );
        REQUIRE( myrates.get_rate( glass_shard, guy ) == -1 );
    }
    WHEN( "item is matched by typeid" ) {
        REQUIRE( myrates.get_rate( item( itype_bow_saw ), guy ) == 2 );
    }
    WHEN( "item is matched by typeid and condition is true" ) {
        guy.set_value( "bool_dinner_bow_saw_eater", "yes" );
        REQUIRE( myrates.get_rate( item( itype_bow_saw ), guy ) == 99 );
    }
    WHEN( "item is matched by category" ) {
        REQUIRE( myrates.get_rate( item( itype_coin_gold ), guy ) == 10 );
    }
    WHEN( "item is matched by category but it's junk" ) {
        REQUIRE( myrates.get_rate( item( itype_coin_dollar ), guy ) == -1 );
    }
    WHEN( "item is matched by item_group" ) {
        REQUIRE( myrates.get_rate( item( itype_test_pipe ), guy ) == 100 );
    }
    WHEN( "item is matched by entry with both item_group and category" ) {
        REQUIRE( myrates.get_rate( item( itype_test_nuclear_carafe ), guy ) == 50 );
    }
    WHEN( "item is matched by category and an overriding entry" ) {
        REQUIRE( myrates.get_rate( item( itype_FMCNote ), guy ) == 25 );
    }
}
