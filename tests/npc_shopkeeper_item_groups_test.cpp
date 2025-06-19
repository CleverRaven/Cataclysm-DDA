#include <list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character_id.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "game.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "npc.h"
#include "npc_class.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_scrap( "scrap" );
static const itype_id itype_test_backpack( "test_backpack" );
static const itype_id itype_test_multitool( "test_multitool" );
static const itype_id itype_test_pants_fur( "test_pants_fur" );

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
    clear_avatar();
    clear_npcs();
    tripoint_bub_ms const npc_pos = get_avatar().pos_bub() + tripoint::east;
    const character_id id = get_map().place_npc( npc_pos.xy(), npc_template_test_npc_trader );
    npc &guy = *g->find_npc( id );

    GIVEN( "item in basic group with no conditions" ) {
        item pants( itype_test_pants_fur );
        pants.set_owner( guy );
        THEN( "item is available for selling and restocking" ) {
            std::pair<bool, bool> har_pants = has_and_can_restock( guy, pants );
            REQUIRE( har_pants.first == true );
            REQUIRE( har_pants.second == true );
            REQUIRE( guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &pants } ) );
        }
    }

    GIVEN( "item in inventory" ) {
        g->load_npcs();
        creature_tracker &creatures = get_creature_tracker();
        REQUIRE( creatures.creature_at<npc>( npc_pos ) != nullptr );
        item backpack( itype_test_backpack );
        backpack.set_owner( guy );
        REQUIRE( guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &backpack } ) );
        WHEN( "backpack is worn - not available for sale" ) {
            auto backpack_iter = *guy.wear_item( backpack );
            item &it = *backpack_iter;
            REQUIRE( !guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &it } ) );
            item scrap( itype_scrap );
            scrap.set_owner( guy );
            REQUIRE( guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &scrap } ) );
            item_location const scrap_inv = guy.i_add( scrap );
            REQUIRE( scrap_inv );
            THEN( "sell_belongings is true - item in inventory available for sale" ) {
                guy.myclass = npc_class_id::NULL_ID();
                REQUIRE( guy.myclass->sells_belongings == true );
                REQUIRE( guy.wants_to_sell( scrap_inv ) );
            }
            THEN( "sell_belongings is false - item in inventory is not available for sale" ) {
                REQUIRE( guy.myclass->sells_belongings == false );
                REQUIRE( !guy.wants_to_sell( scrap_inv ) );
            }
        }
    }

    GIVEN( "item in group gated by non-strict condition" ) {
        item hammer( itype_hammer );
        hammer.set_owner( guy );
        WHEN( "condition not met" ) {
            std::pair<bool, bool> har_hammer = has_and_can_restock( guy, hammer );
            THEN( "item is available for restocking but not selling" ) {
                REQUIRE( har_hammer.first == true );
                REQUIRE( har_hammer.second == true );
                REQUIRE( !guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &hammer } ) );
            }
        }
        WHEN( "condition met" ) {
            get_avatar().set_value( "bool_test_tools_access", "yes" );
            std::pair<bool, bool> har_hammer = has_and_can_restock( guy, hammer );
            THEN( "item is available for selling and restocking" ) {
                REQUIRE( har_hammer.first == true );
                REQUIRE( har_hammer.second == true );
                REQUIRE( guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &hammer } ) );
            }
        }
    }

    GIVEN( "item in group gated by strict condition" ) {
        item multitool( itype_test_multitool );
        multitool.set_owner( guy );
        WHEN( "condition not met" ) {
            std::pair<bool, bool> har_multitool = has_and_can_restock( guy, multitool );
            THEN( "item is not available for selling or restocking" ) {
                REQUIRE( har_multitool.first == true );
                REQUIRE( har_multitool.second == false );
                REQUIRE( !guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &multitool } ) );
            }
        }
        WHEN( "condition met" ) {
            get_avatar().set_value( "bool_test_multitool_access", "yes" );
            std::pair<bool, bool> har_multitool = has_and_can_restock( guy, multitool );
            THEN( "item is available for selling and restocking" ) {
                REQUIRE( har_multitool.first == true );
                REQUIRE( har_multitool.second == true );
                REQUIRE( guy.wants_to_sell( { map_cursor{ tripoint_bub_ms::zero }, &multitool } ) );
            }
        }
    }

    GIVEN( "containter with single item type and conditions only for contents" ) {
        item multitool( itype_test_multitool );
        item bag( itype_bag_plastic );
        int const num = GENERATE( 1, 2 );
        bool ret = true;
        for( int i = 0; i < num; i++ ) {
            ret &= bag.put_in( multitool, pocket_type::CONTAINER ).success();
        }
        CAPTURE( num, bag.display_name() );
        REQUIRE( ret );
        bag.set_owner( guy );
        item_location const loc( map_cursor{ tripoint_bub_ms::zero}, &bag );
        WHEN( "condition for contents not met" ) {
            THEN( "container can't be sold" ) {
                REQUIRE( !guy.wants_to_sell( loc ) );
            }
        }
        WHEN( "condition for contents met" ) {
            get_avatar().set_value( "bool_test_multitool_access", "yes" );
            THEN( "container can be sold" ) {
                REQUIRE( guy.wants_to_sell( loc ) );
            }
        }
    }
}
