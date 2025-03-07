#include <string>
#include <vector>
#include <map>
#include <utility>

#include "advanced_inv.h"
#include "advanced_inv_area.h"
#include "advanced_inv_pane.h"
#include "avatar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "coords_fwd.h"
#include "coordinates.h"
#include "item_location.h"
#include "item.h"
#include "map_helpers.h"
#include "map.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "rng.h"
#include "type_id.h"


static const itype_id itype_backpack( "backpack" );
static const itype_id itype_knife_combat( "knife_combat" );
static const itype_id itype_test_9mm_ammo( "test_9mm_ammo" );


/*
    --------- AIM testing ----------
    TODO: add more tests
    To setup the panes, call @ref init_panes with the source and destination @ref aim_location you want to test.
    After any changes to the items (spawning in) call @ref recalc_panes.
*/

// the required action to select desired location
static const std::map<aim_location, const std::string> loc_action {
    {aim_location::AIM_INVENTORY, "ITEMS_INVENTORY"},
    {aim_location::AIM_SOUTHWEST, "ITEMS_SW"},
    {aim_location::AIM_SOUTH, "ITEMS_S"},
    {aim_location::AIM_SOUTHEAST, "ITEMS_SE"},
    {aim_location::AIM_WEST, "ITEMS_W"},
    {aim_location::AIM_CENTER, "ITEMS_CE"},
    {aim_location::AIM_EAST, "ITEMS_E"},
    {aim_location::AIM_NORTHWEST, "ITEMS_NW"},
    {aim_location::AIM_NORTH, "ITEMS_N"},
    {aim_location::AIM_NORTHEAST, "ITEMS_NE"},
    {aim_location::AIM_DRAGGED, "ITEMS_DRAGGED_CONTAINER"},
    {aim_location::AIM_ALL, "ITEMS_AROUND"},
    {aim_location::AIM_CONTAINER, "ITEMS_CONTAINER"},
    {aim_location::AIM_PARENT, "ITEMS_PARENT"},
    {aim_location::AIM_WORN, "ITEMS_WORN"}
};


// recalc SECTION anything in the panels changes (item added etc.)
static void recalc_panes( advanced_inventory &advinv )
{
    advinv.recalc_pane( advanced_inventory::side::left );
    advinv.recalc_pane( advanced_inventory::side::right );
}

static void init_panes( advanced_inventory &advinv, aim_location sloc,
                        aim_location dloc )
{
    advanced_inventory::side src = advinv.get_src();
    if( advinv.get_pane( src ).get_area() != dloc ) {
        advinv.process_action( loc_action.at( dloc ) );
        REQUIRE( advinv.get_pane( src ).get_area() == dloc );
    }
    advinv.process_action( "TOGGLE_TAB" );
    src = advinv.get_src();
    if( advinv.get_pane( src ).get_area() != sloc ) {
        advinv.process_action( loc_action.at( sloc ) );
        REQUIRE( advinv.get_pane( src ).get_area() == sloc );
    }
    recalc_panes( advinv );
}

static void do_activity( advanced_inventory &advinv, const std::string &activity )
{
    avatar &u = get_avatar();
    advinv.process_action( activity );
    process_activity( u );
    REQUIRE_FALSE( u.activity );
    recalc_panes( advinv );
}

TEST_CASE( "AIM_basic_move_items", "[items][advanced_inv]" )
{

    avatar &u = get_avatar();
    clear_avatar();
    clear_map();
    advanced_inventory advinv;
    advinv.init();

    map &here = get_map();
    tripoint_bub_ms pos = u.pos_bub();

    item backpack( itype_backpack );
    item knife_combat( itype_knife_combat );
    item i_9mm_ammo( itype_test_9mm_ammo );

    const units::volume max_vol = backpack.get_total_capacity();
    const units::mass max_mass = backpack.get_total_weight_capacity();

    SECTION( "from ground to inv" ) {


        // an item is in the src panel and is selected
        init_panes( advinv, aim_location::AIM_CENTER, aim_location::AIM_INVENTORY );
        advanced_inventory::side src = advinv.get_src();
        advanced_inventory_pane &spane = advinv.get_pane( src );

        GIVEN( "a single item on the ground" ) {
            item &knife_combat_map = get_map().add_item_or_charges( pos, knife_combat );


            // THEN( "there is an item in the srcpanel" ) {
            recalc_panes( advinv );
            REQUIRE_FALSE( here.i_at( pos ).empty() );
            REQUIRE_FALSE( advinv.get_pane( advinv.get_src() ).items.empty() );
            // }
            // AND_THEN( "the pointer points to an item" ) {
            REQUIRE( spane.get_cur_item_ptr() );
            // }

            std::string knife_combat_uid = random_string( 10 );
            knife_combat_map.set_var( "uid", knife_combat_uid );

            AND_GIVEN( "there is not enough space in the inventory" ) {
                REQUIRE_FALSE( u.can_stash( knife_combat_map ) );

                WHEN( "trying to move the item" ) {
                    THEN( "item does not get transferred" ) {
                        CHECK_FALSE( player_has_item_of_type( itype_knife_combat ) );
                    }

                    AND_THEN( "Item is still on the ground" ) {
                        CHECK_FALSE( spane.items.empty() );
                    }
                }
            }

            AND_GIVEN( "there is enough space in the inventory" ) {
                u.worn.wear_item( u, backpack, false, false );
                REQUIRE( u.can_stash( knife_combat_map ) );

                WHEN( "trying to move the item" ) {
                    do_activity( advinv, "MOVE_SINGLE_ITEM" );

                    THEN( "item is in player inventory" ) {
                        CHECK( character_has_item_with_var_val( u, "uid",
                                                                knife_combat_uid ) );
                    }

                    AND_THEN( "item is no longer on the ground" ) {
                        CHECK( spane.items.empty() );
                    }
                }
            }
        }

        GIVEN( "multiple items in stack on the ground" ) {
            // don't need to test case full inv again.
            u.worn.wear_item( u, backpack, false, false );

            REQUIRE( u.top_items_loc().size() == 1 );   // backpack
            item_location bp_worn = u.top_items_loc().back();
            REQUIRE( bp_worn->is_container_empty() );

            const int num_items = 10000;

            GIVEN( "items are not charges" ) {
                item &knife_combat_map = get_map().add_item_or_charges( u.pos_bub(), knife_combat, num_items );
                REQUIRE( get_map().i_at( u.pos_bub() ).size() == num_items );

                recalc_panes( advinv );

                THEN( "items are stacked properly" ) {
                    REQUIRE( spane.items.size() == 1 );
                    REQUIRE( spane.get_cur_item_ptr()->stacks == num_items );
                }

                const int max_capacity = std::min(
                                             knife_combat_map.charges_per_volume( max_vol ),
                                             knife_combat_map.charges_per_weight( max_mass )
                                         );

                AND_GIVEN( "you can stash some, but not all items " ) {
                    REQUIRE( max_capacity > num_items );
                    REQUIRE( u.can_stash( knife_combat, max_capacity ) );
                    REQUIRE_FALSE( u.can_stash( knife_combat, max_capacity + 1 ) );
                }

                AND_WHEN( "transfering single item" ) {
                    do_activity( advinv, "MOVE_SINGLE_ITEM" );
                    THEN( "a single item is in inventory" ) {
                        CHECK( u.has_amount( itype_knife_combat, 1 ) );
                    }
                    AND_THEN( "a single item is removed from src" ) {
                        CHECK( spane.items.size() == num_items - 1 );
                    }
                }

                WHEN( "transfering variable items" ) {
                    // because we can't input an amount, it will transfer max_possible
                    do_activity( advinv, "MOVE_VARIABLE_ITEM" );

                    THEN( "max_capacity number of items should be transfered" ) {
                        CHECK( u.has_amount( itype_knife_combat, max_capacity ) );
                    }
                    AND_THEN( "a single item is removed from src" ) {
                        CHECK( spane.items.size() == num_items - max_capacity );
                    }
                }

                WHEN( "transfering item stack" ) {
                    // because we can't input an amount, it will transfer max_possible
                    do_activity( advinv, "MOVE_VARIABLE_ITEM" );

                    THEN( "max_capacity items are in inventory" ) {
                        CHECK( u.has_amount( itype_knife_combat, max_capacity ) );
                    }
                    AND_THEN( "max_capacity items are removed from src" ) {
                        CHECK( spane.items.size() == num_items - max_capacity );
                    }
                    AND_THEN( "no more items can be transfered" ) {
                        CHECK_FALSE( u.can_stash( knife_combat, 1 ) );
                        CHECK_FALSE( u.try_add( knife_combat, /**avoid=*/ nullptr,
                                                /**original_inventory_item=*/ nullptr, /*allow_wield=*/ false ) );
                    }
                }
            }

            GIVEN( "items are charges" ) {
                item &map_i_9mm_ammo = here.add_item_or_charges( pos, i_9mm_ammo, num_items );
                recalc_panes( advinv );

                const int max_capacity = std::min(
                                             map_i_9mm_ammo.charges_per_volume( max_vol ),
                                             map_i_9mm_ammo.charges_per_weight( max_mass )
                                         );
                REQUIRE( max_capacity < num_items );
                REQUIRE( map_i_9mm_ammo.count_by_charges() );

                WHEN( "transfering single item" ) {
                    do_activity( advinv, "MOVE_SINGLE_ITEM" );
                    THEN( "all charges are in inventory" ) {
                        CHECK( u.has_amount( itype_test_9mm_ammo, max_capacity ) );
                        CHECK_FALSE( u.has_amount( itype_test_9mm_ammo, max_capacity + 1 ) );
                    }
                    AND_THEN( "all charges are removed from src" ) {
                        CHECK( spane.get_cur_item_ptr()->contents_count == num_items - max_capacity );
                    }
                }
                WHEN( "transfering variable items" ) {
                    // because we can't input an amount, it will transfer max_possible
                    do_activity( advinv, "MOVE_VARIABLE_ITEM" );

                    THEN( "max_capacity number of items should be transfered" ) {
                        CHECK( u.has_amount( itype_test_9mm_ammo, max_capacity ) );
                    }
                    AND_THEN( "max_capacity charges removed from src" ) {
                        CHECK( spane.get_cur_item_ptr()->contents_count == num_items - max_capacity );
                    }
                }

                WHEN( "transfering item stack" ) {
                    // because we can't input an amount, it will transfer max_possible
                    do_activity( advinv, "MOVE_VARIABLE_ITEM" );

                    THEN( "max_capacity items are in inventory" ) {
                        CHECK( u.has_amount( itype_test_9mm_ammo, max_capacity ) );
                    }
                    AND_THEN( "max_capacity items are removed from src" ) {
                        CHECK( spane.items.size() == num_items - max_capacity );
                    }
                    AND_THEN( "no more items can be transfered" ) {
                        CHECK_FALSE( u.can_stash( i_9mm_ammo, 1 ) );
                        CHECK_FALSE( u.try_add( i_9mm_ammo, /*avoid=*/ nullptr,
                                                /*original_inventory_item=*/ nullptr, /*allow_wield=*/ false ) );
                    }
                }
            }
        }
    }
}
