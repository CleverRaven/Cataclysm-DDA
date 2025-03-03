#include "advanced_inv.h"
#include "advanced_inv_area.h"
#include "advanced_inv_pane.h"
#include "avatar.h"
#include "cata_catch.h"
#include "game.h"
#include "player_helpers.h"
#include "map.h"
#include "map_selector.h"
#include "map_helpers.h"
#include "item.h"
#include "uistate.h"

#include <memory>


static const itype_id itype_backpack( "backpack" );
static const itype_id itype_knife_combat( "knife_combat" );

/*
    AIM testing
*/



TEST_CASE( "advanced_inventory_test", "[advanced_inventory][item]" )
{
    clear_avatar();
    std::unique_ptr<advanced_inventory> advinv = std::make_unique<advanced_inventory>();
    advinv->init();

    SECTION( "check panes" ) {
        SECTION( "init" ) {
            CHECK( advinv );
        }
        // SECTION( "move_items" ) {

        //     CHECK( all_sq.id );

        // }

    }

    SECTION( "move_item_test" ) {

    }

}

TEST_CASE( "advanced_inventory_actions" )
{
    clear_avatar();
    std::unique_ptr<advanced_inventory> advinv = std::make_unique<advanced_inventory>();
    advinv->init();
    avatar u;
    u.set_body();
    item backpack( itype_backpack );
    item knife_combat( itype_knife_combat );

    //auto item_iter =
    u.worn.wear_item( u, backpack, false, false );
    advanced_inventory::side src = advinv->get_src();


    SECTION( "move_single_item" ) {
        SECTION( "ground_to_inv" ) {

            item &item_small = get_map().add_item_or_charges( u.pos_bub(), knife_combat );
            REQUIRE( backpack.can_contain( item_small ).success() );
            item_location item_loc( map_cursor( u.pos_abs() ), &item_small );

            WHEN( "src is ground and dest is inv" ) {
                // TODO: make sure state doesnt start with any of those, since it would get swapped
                REQUIRE( advinv->get_pane( src ).get_area() !=
                         aim_location::AIM_INVENTORY );

                if( advinv->get_pane( src ).get_area() != aim_location::AIM_INVENTORY ) {
                    advinv->process_action( "ITEMS_INVENTORY" );
                }
                advinv->process_action( "TOGGLE_TAB" );
                src = advinv->get_src();
                if( advinv->get_pane( src ).get_area() != aim_location::AIM_CENTER ) {
                    advinv->process_action( "ITEMS_CE" );
                }

                advinv->recalc_pane( src );
                REQUIRE_FALSE( advinv->get_pane( src ).items.empty() );
                advanced_inv_listitem *ptr = advinv->get_pane( src ).get_cur_item_ptr();
                REQUIRE( ptr );
                WHEN( "inventory has enough space" ) {

                    THEN( "item gets transferred" ) {
                        advinv->process_action( "MOVE_SINGLE_ITEM" );
                        CHECK( u.has_item( knife_combat ) );
                    }
                }

            }

        }
    }

}
