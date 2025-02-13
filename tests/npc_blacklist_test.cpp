#include <string>

#include "cata_catch.h"
#include "item.h"
#include "npc.h"
#include "type_id.h"

static const itype_id itype_bow_saw( "bow_saw" );

static npc_template_id const npc_template_test_npc_trader( "test_npc_trader" );

TEST_CASE( "npc_blacklist", "[npc][trade]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_trader );

    REQUIRE( guy.wants_to_buy( item( itype_bow_saw ) ) );
    guy.set_value( "bool_bigotry_hates_bow_saws", "yes" );
    REQUIRE( !guy.wants_to_buy( item( itype_bow_saw ) ) );
}
