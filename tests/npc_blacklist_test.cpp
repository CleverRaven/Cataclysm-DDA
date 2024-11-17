#include "cata_catch.h"
#include "npc.h"

static npc_template_id const npc_template_test_npc_trader( "test_npc_trader" );

TEST_CASE( "npc_blacklist", "[npc][trade]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_trader );

    item saw( "bow_saw" );
    item_location saw_loc( map_cursor{ tripoint_bub_ms{} }, &saw );
    REQUIRE( guy.wants_to_buy( saw_loc ) );
    guy.set_value( "bool_bigotry_hates_bow_saws", "yes" );
    CHECK( !guy.wants_to_buy( saw_loc ) );
}
