#include "avatar.h"
#include "cata_catch.h"
#include "item_group.h"
#include "npc.h"
#include "npc_class.h"

static npc_template_id const npc_template_test_npc_trader( "test_npc_trader" );

static std::pair<bool, bool> has_and_can_restock( npc const &guy, item const &it )
{
    bool can_restock_item = false;
    bool item_in_groups = false;
    for( shopkeeper_item_group const &ig : guy.myclass->get_shopkeeper_items() ) {
        if( item_group::group_contains_item( ig.id, it.typeId() ) ) {
            item_in_groups = true;
            can_restock_item |= ig.can_restock( guy );
        }
    }
    return { item_in_groups, can_restock_item };
}

TEST_CASE( "npc_shopkeeper_item_groups", "[npc][trade]" )
{
    npc guy;
    guy.load_npc_template( npc_template_test_npc_trader );

    GIVEN( "item in basic group with no conditions" ) {
        item pants( "test_pants_fur" );
        pants.set_owner( guy );
        THEN( "item is available for selling and restocking" ) {
            std::pair<bool, bool> har_pants = has_and_can_restock( guy, pants );
            REQUIRE( har_pants.first == true );
            REQUIRE( har_pants.second == true );
            REQUIRE( guy.wants_to_sell( pants ) );
        }
    }

    GIVEN( "item in group gated by non-strict condition" ) {
        item hammer( "hammer" );
        hammer.set_owner( guy );
        WHEN( "condition not met" ) {
            std::pair<bool, bool> har_hammer = has_and_can_restock( guy, hammer );
            THEN( "item is available for restocking but not selling" ) {
                REQUIRE( har_hammer.first == true );
                REQUIRE( har_hammer.second == true );
                REQUIRE( !guy.wants_to_sell( hammer ) );
            }
        }
        WHEN( "condition met" ) {
            get_avatar().set_value( "npctalk_var_bool_test_tools_access", "yes" );
            std::pair<bool, bool> har_hammer = has_and_can_restock( guy, hammer );
            THEN( "item is available for selling and restocking" ) {
                REQUIRE( har_hammer.first == true );
                REQUIRE( har_hammer.second == true );
                REQUIRE( guy.wants_to_sell( hammer ) );
            }
        }
    }

    GIVEN( "item in group gated by strict condition" ) {
        item multitool( "test_multitool" );
        multitool.set_owner( guy );
        WHEN( "condition not met" ) {
            std::pair<bool, bool> har_multitool = has_and_can_restock( guy, multitool );
            THEN( "item is not available for selling or restocking" ) {
                REQUIRE( har_multitool.first == true );
                REQUIRE( har_multitool.second == false );
                REQUIRE( !guy.wants_to_sell( multitool ) );
            }
        }
        WHEN( "condition met" ) {
            get_avatar().set_value( "npctalk_var_bool_test_multitool_access", "yes" );
            std::pair<bool, bool> har_multitool = has_and_can_restock( guy, multitool );
            THEN( "item is available for selling and restocking" ) {
                REQUIRE( har_multitool.first == true );
                REQUIRE( har_multitool.second == true );
                REQUIRE( guy.wants_to_sell( multitool ) );
            }
        }
    }
}
