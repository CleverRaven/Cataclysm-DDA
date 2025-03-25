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

/*
    --------- AIM testing ----------
    TODO: add more tests
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


// recalc when anything in the panels changes (item added etc.)
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

TEST_CASE( "advanced_inventory_navigation" )
{
    clear_avatar();
    clear_map();
    advanced_inventory advinv;

    advinv.init();
    advanced_inventory::side init_src = advinv.get_src();
    advanced_inventory::side init_dest = advinv.get_dest();
    advanced_inventory_pane init_src_pane = advinv.get_pane( init_src );
    advanced_inventory_pane init_dest_pane = advinv.get_pane( init_dest );

    GIVEN( "Areas are not the same" ) {
        REQUIRE( init_src_pane.get_area() != init_dest_pane.get_area() );

        WHEN( "Switching sides" ) {
            advinv.process_action( "TOGGLE_TAB" );

            advanced_inventory::side src = advinv.get_src();
            advanced_inventory::side dest = advinv.get_dest();
            advanced_inventory_pane src_pane = advinv.get_pane( src );
            advanced_inventory_pane dest_pane = advinv.get_pane( dest );

            THEN( "sides got switched" ) {
                REQUIRE( src == init_dest );
                REQUIRE( dest == init_src );
            }
            AND_THEN( "pane areas got switched" ) {
                REQUIRE( src_pane.get_area() == init_dest_pane.get_area() );
                REQUIRE( dest_pane.get_area() == init_src_pane.get_area() );

            }
        }
    }
}

TEST_CASE( "advanced_inventory_actions" )
{
    clear_avatar();
    clear_map();
    advanced_inventory advinv;
    advinv.init();
    avatar &u = get_avatar();

    item backpack( itype_backpack );
    item knife_combat( itype_knife_combat );

    SECTION( "move_single_item" ) {
        item &knife_combat_map = get_map().add_item_or_charges( u.pos_bub(), knife_combat );
        std::string knife_combat_uid = random_string( 10 );
        knife_combat_map.set_var( "uid", knife_combat_uid );
        item_location item_loc_knife( map_cursor( u.pos_abs() ), &knife_combat_map );

        SECTION( "ground_to_inv" ) {
            init_panes( advinv, aim_location::AIM_CENTER, aim_location::AIM_INVENTORY );

            // an item is in the src panel and is selected
            advanced_inventory::side src = advinv.get_src();
            REQUIRE_FALSE( advinv.get_pane( src ).items.empty() );
            REQUIRE( advinv.get_pane( src ).get_cur_item_ptr() );

            WHEN( "there is enough space in the inventory" ) {
                u.worn.wear_item( u, backpack, false, false );
                REQUIRE( u.can_stash( knife_combat_map ) );

                do_activity( advinv, "MOVE_SINGLE_ITEM" );

                THEN( "item gets transferred" ) {
                    CHECK( character_has_item_with_var_val( u, "uid",
                                                            knife_combat_uid ) );
                }
            }

            WHEN( "there is not enough space in the inventory" ) {
                REQUIRE_FALSE( u.can_stash( knife_combat_map ) );

                THEN( "item does not get transferred" ) {
                    CHECK_FALSE( player_has_item_of_type( itype_knife_combat ) );
                }

                THEN( "Item is still on the ground" ) {
                    CHECK_FALSE( advinv.get_pane( src ).items.empty() );
                }
            }
        }
    }
}
