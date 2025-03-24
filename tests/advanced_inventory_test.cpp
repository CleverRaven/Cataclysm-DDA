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
static const itype_id itype_debug_heavy_backpack( "test_heavy_debug_backpack" );
static const itype_id itype_debug_backpack( "debug_backpack" );
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

/* this should mirror what query_charges returns as max items when transferring to inventory */
/* this should mirror what query_charges returns as max items when transferring to inventory */
static int u_carry_amount( item &it )
{
    int amount = INT_MAX;
    avatar &player_character = get_avatar();

    player_character.can_stash_partial( it, amount );


    player_character.can_stash_partial( it, amount );

    const units::mass unitweight = it.weight() / ( it.count_by_charges() ? it.charges : 1 );
    if( unitweight > 0_gram ) {

        const units::mass overburden_capacity = player_character.weight_capacity() * 4 -
                                                player_character.weight_carried();

        // TODO: have it consider pocket weight_multiplier
        const int weightmax = overburden_capacity / unitweight;

        const units::mass overburden_capacity = player_character.weight_capacity() * 4 -
                                                player_character.weight_carried();

        // TODO: have it consider pocket weight_multiplier
        const int weightmax = overburden_capacity / unitweight;
        if( weightmax <= 0 ) {
            return 0;
        }
        amount = std::min( weightmax, amount );
    }


    return amount;
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
    item debug_heavy_backpack( itype_debug_backpack );
    item debug_backpack( itype_debug_backpack );
    item debug_heavy_backpack( itype_debug_backpack );
    item debug_backpack( itype_debug_backpack );
    item knife_combat( itype_knife_combat );
    item i_9mm_ammo( itype_test_9mm_ammo );

    // const units::volume max_bp_vol = backpack.get_total_capacity();
    // const units::mass max_bp_mass = backpack.get_total_weight_capacity();

    // const units::volume max_dbg_bp_vol = debug_backpack.get_total_capacity();
    // const units::mass max_dbg_bp_mass = debug_backpack.get_total_weight_capacity();

    // const units::volume max_bp_vol = backpack.get_total_capacity();
    // const units::mass max_bp_mass = backpack.get_total_weight_capacity();

    // const units::volume max_dbg_bp_vol = debug_backpack.get_total_capacity();
    // const units::mass max_dbg_bp_mass = debug_backpack.get_total_weight_capacity();


    // this is directly from advanced_inventory:query_charges,
    // we probably want to generalize what counts as overburdened
    const units::mass overburden_mass = u.weight_capacity() * 4;

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

            GIVEN( "there is not enough space in the inventory" ) {
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

            AND_GIVEN( "there is enough space in the inventory for some" ) {
                AND_GIVEN( "there is enough space in the inventory for some" ) {
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
        }

        GIVEN( "multiple items in stack on the ground" ) {
            const int limit = here.i_at( pos ).count_limit();
            int remaining_map = limit;

            GIVEN( "items are not charges" ) {
                item &knife_combat_map = here.add_item_or_charges( pos, knife_combat,
                                         remaining_map,
                                         remaining_map,
                                         false );
                const int num_items = limit - remaining_map;
                int remaining_stash = num_items;
                int remaining_stash = num_items;

                AND_GIVEN( "all items got placed" ) {
                    REQUIRE( here.i_at( pos ).size() == num_items );
                    REQUIRE( remaining_map == 0 );
                    REQUIRE( remaining_map == 0 );
                }

                recalc_panes( advinv );

                AND_THEN( "items are stacked properly" ) {
                    REQUIRE( spane.items.size() == 1 );
                    REQUIRE( spane.get_cur_item_ptr()->stacks == num_items );
                }

                AND_GIVEN( "there is enough space for some, but not all items" ) {
                    u.worn.wear_item( u, backpack, false, false );


                    REQUIRE( u.can_stash_partial( knife_combat_map, remaining_stash ) );
                    REQUIRE( remaining_stash > 0 );

                    const int expected_transfered = u_carry_amount( knife_combat );
                    REQUIRE( expected_transfered < num_items );
                    const int expected_remaining = num_items - expected_transfered;




                    WHEN( "transfering single item" ) {
                        do_activity( advinv, "MOVE_SINGLE_ITEM" );
                        THEN( "a single item is in inventory" ) {
                            CHECK( u.has_amount( itype_knife_combat, 1 ) );
                        }
                        AND_THEN( "a single item is removed from src" ) {
                            CHECK( spane.get_cur_item_ptr()->stacks == num_items - 1 );
                        }
                    }
                    WHEN( "transfering single item" ) {
                        do_activity( advinv, "MOVE_SINGLE_ITEM" );
                        THEN( "a single item is in inventory" ) {
                            CHECK( u.has_amount( itype_knife_combat, 1 ) );
                        }
                        AND_THEN( "a single item is removed from src" ) {
                            CHECK( spane.get_cur_item_ptr()->stacks == num_items - 1 );
                        }
                    }

                    WHEN( "transfering variable items" ) {
                        // because we can't input an amount, it will transfer max_possible
                        do_activity( advinv, "MOVE_VARIABLE_ITEM" );
                        WHEN( "transfering variable items" ) {
                            // because we can't input an amount, it will transfer max_possible
                            do_activity( advinv, "MOVE_VARIABLE_ITEM" );

                            THEN( "an amount of items should be transfered" ) {
                                CHECK( u.amount_of( itype_knife_combat ) == expected_transfered );
                            }
                            AND_THEN( "you are not overburdened" ) {
                                CHECK( overburden_mass > u.weight_carried() );
                            }

                            // you are volume restricted, not overburden restricted
                            AND_THEN( "you cannot fit more" ) {
                                CHECK_FALSE( u.can_stash( knife_combat ) );
                            }
                        }
                        // you are volume restricted, not overburden restricted
                        AND_THEN( "you cannot fit more" ) {
                            CHECK_FALSE( u.can_stash( knife_combat ) );
                        }
                    }

                    WHEN( "transfering item stack" ) {
                        // because we can't input an amount, it will transfer max_possible
                        do_activity( advinv, "MOVE_VARIABLE_ITEM" );
                        WHEN( "transfering item stack" ) {
                            // because we can't input an amount, it will transfer max_possible
                            do_activity( advinv, "MOVE_VARIABLE_ITEM" );

                            THEN( "you have that amount of items are in inventory" ) {
                                CHECK( u.amount_of( itype_knife_combat ) == expected_transfered );
                            }
                            AND_THEN( "some items are removed from src" ) {
                                CHECK( spane.get_cur_item_ptr()->stacks == expected_remaining );
                            }
                            AND_THEN( "you are not overburdened" ) {
                                CHECK( overburden_mass > u.weight_carried() );
                            }
                            AND_THEN( "no more items can be transfered" ) {
                                CHECK_FALSE( u.can_stash( knife_combat, 1 ) );
                                CHECK_FALSE( u.try_add( knife_combat, /**avoid=*/ nullptr,
                                                        /**original_inventory_item=*/ nullptr, /*allow_wield=*/ false ) );
                            }
                        }
                    }

                    GIVEN( "there is enough space for all items" ) {
                        u.worn.wear_item( u, debug_backpack, false, false );
                        REQUIRE( u.can_stash_partial( knife_combat_map, remaining_stash ) );
                        REQUIRE( remaining_stash == 0 );

                        int can_carry = u_carry_amount( knife_combat );

                        // TODO: currently pocket weight_multiplier do not get considered for overburden
                        // when that changes please reverse this
                        REQUIRE( can_carry < num_items );

                        WHEN( "transfering item stack" ) {
                            // because we can't input an amount, it will transfer max_possible
                            do_activity( advinv, "MOVE_VARIABLE_ITEM" );

                            THEN( "some items are in inventory" ) {
                                CHECK( u.amount_of( itype_knife_combat ) > 1 );
                            }
                            AND_THEN( "some items are removed from src" ) {
                                CHECK( spane.items.size() < num_items );
                            }
                            AND_THEN( "you are not overburdened" ) {
                                CHECK( overburden_mass > u.weight_carried() );
                            }
                        }
                    }
                }
            }

            GIVEN( "items are charges" ) {
                item &map_i_9mm_ammo = here.add_item_or_charges( pos, i_9mm_ammo, remaining_map, false );
                const int num_items = limit - remaining_map;
                REQUIRE( map_i_9mm_ammo.count_by_charges() );
                recalc_panes( advinv );

                const units::mass unitweight = i_9mm_ammo.weight() / i_9mm_ammo.charges;
                REQUIRE( unitweight > 0_gram );
                int left_over = num_items;

                GIVEN( "there is enough space for some, but not all items" ) {
                    GIVEN( "pocket weight capacity is the limiting factor" ) {
                        // we should* be limited by being overburdened.
                        u.worn.wear_item( u, backpack, false, false );
                        const int expected_transfered = u_carry_amount( knife_combat );
                        REQUIRE( expected_transfered < num_items );
                        const int expected_remaining = num_items - expected_transfered;
                        u.can_stash_partial( i_9mm_ammo, left_over );
                        REQUIRE( left_over > 0 );

                        // they should behave exactly the same for charges (atleast until you can enter an amount in query_charges)
                        for( std::string activity : {
                                 "MOVE_SINGLE_ITEM", "MOVE_VARIABLE_ITEM", "MOVE_ITEM_STACK"
                             } ) {
                            WHEN( "transfering " + activity ) {
                                do_activity( advinv, "MOVE_SINGLE_ITEM" );
                                THEN( "number of charges are in inventory" ) {
                                    CHECK( u.has_charges( itype_test_9mm_ammo, expected_transfered ) );
                                }
                                AND_THEN( "number of charges are removed from src" ) {
                                    CHECK( spane.get_cur_item_ptr()->items.front()->charges ==
                                           expected_remaining );
                                    CHECK( map_i_9mm_ammo.charges == expected_remaining );
                                }
                            }
                        }
                    }
                    GIVEN( "overburden is the constraining factor" ) {

                        // has to be an item that fits all items and has a higher pocket weight capacity then the character,
                        // but still transfers weight to character. No current item fits that, so make a custom one.
                        u.worn.wear_item( u, debug_heavy_backpack, false, false );

                        REQUIRE( u.can_stash_partial( i_9mm_ammo, left_over ) );
                        // can stash all items ignoring overburden
                        REQUIRE( left_over == 0 );

                        const units::mass overburden_capacity = u.weight_capacity() * 4 -
                                                                u.weight_carried();
                        REQUIRE( map_i_9mm_ammo.weight() > overburden_capacity );
                        const int num_until_overburden = overburden_capacity / unitweight;
                        REQUIRE( num_until_overburden < num_items );

                        WHEN( "transfering all" ) {
                            do_activity( advinv, "MOVE_ITEM_STACK" );

                            THEN( num_until_overburden + " items get transfered" ) {
                                CHECK( u.has_charges( itype_test_9mm_ammo, num_until_overburden ) );
                                CHECK_FALSE( u.has_charges( itype_test_9mm_ammo, num_until_overburden + 1 ) );
                            }

                        }
                    }

                }
                GIVEN( "you can fit all items" ) {
                    // debug backpack has weight_multiplier of 0.01, so overburden is not the constraining factor
                    u.worn.wear_item( u, debug_backpack, false, false );

                    u.can_stash_partial( i_9mm_ammo, left_over );
                    REQUIRE( leftover == 0 );

                    WHEN( "transfering all" ) {
                        do_activity( advinv, "MOVE_ITEM_STACK" );

                        THEN( num_until_overburden + " items get transfered" ) {
                            CHECK( u.has_charges( itype_test_9mm_ammo, num_until_overburden ) );
                            CHECK_FALSE( u.has_charges( itype_test_9mm_ammo, num_until_overburden + 1 ) );
                        }

                    }

                }
            }

        }

    }
}