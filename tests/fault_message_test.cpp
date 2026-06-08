#include <list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "fault.h"
#include "item.h"
#include "item_location.h"
#include "map_helpers.h"
#include "messages.h"
#include "npc.h"
#include "player_helpers.h"
#include "string_formatter.h"
#include "type_id.h"

static const fault_id fault_cotton_torn( "fault_cotton_torn" );

static const itype_id itype_jacket_leather( "jacket_leather" );

TEST_CASE( "fault_message_substitution_and_gating", "[fault][item][degrade]" )
{
    clear_avatar();
    clear_map();
    avatar &av = get_avatar();
    Messages::clear_messages();

    item jacket( itype_jacket_leather );

    SECTION( "ground item with no holder is silent" ) {
        jacket.set_fault( fault_cotton_torn, true, nullptr );
        CHECK( Messages::size() == 0 );
    }

    SECTION( "avatar-worn item emits substituted message" ) {
        std::optional<std::list<item>::iterator> worn_it =
            av.wear_item( jacket, false );
        REQUIRE( worn_it.has_value() );
        item &worn = **worn_it;
        REQUIRE( av.has_item( worn ) );
        Messages::clear_messages();

        worn.set_fault( fault_cotton_torn, true, &av );

        REQUIRE( Messages::size() == 1 );
        const std::string expected =
            string_format( fault_cotton_torn->message(), worn.tname() );
        CHECK( Messages::recent_messages( 1 ).back().second == expected );
        // %s must have been substituted; raw "%s" should not appear.
        CHECK( expected.find( "%s" ) == std::string::npos );
    }

    SECTION( "avatar passed but item not carried is silent" ) {
        // Same item instance, never added to avatar inventory.
        item lone_jacket( itype_jacket_leather );
        REQUIRE_FALSE( av.has_item( lone_jacket ) );
        Messages::clear_messages();

        lone_jacket.set_fault( fault_cotton_torn, true, &av );
        CHECK( Messages::size() == 0 );
    }

    SECTION( "npc-held item does not spam avatar log" ) {
        standard_npc dude( "TestDude" );
        item_location npc_jacket = dude.i_add( item( itype_jacket_leather ) );
        REQUIRE( npc_jacket );
        Messages::clear_messages();

        npc_jacket.get_item()->set_fault( fault_cotton_torn, true, &dude );
        CHECK( Messages::size() == 0 );
    }
}
