#include "cata_catch.h"
#include "npc.h"

static npc_template_id const npc_template_test_npc_trader( "test_npc_trader" );

TEST_CASE( "npc_blacklist", "[npc][trade]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_trader );

    REQUIRE( guy.wants_to_buy( item( "bow_saw" ) ) );
    guy.set_value( "bool_bigotry_hates_bow_saws", "yes" );
    REQUIRE( !guy.wants_to_buy( item( "bow_saw" ) ) );
}
