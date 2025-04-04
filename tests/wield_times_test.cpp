#include <list>
#include <optional>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "coordinates.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "npc.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_knife_combat( "knife_combat" );
static const itype_id itype_knife_hunting( "knife_hunting" );
static const itype_id itype_metal_tank( "metal_tank" );
static const itype_id itype_pants_cargo( "pants_cargo" );
static const itype_id itype_sheath( "sheath" );

static constexpr point_bub_ms player_pos{ 50, 50 };

static void wield_check_from_inv( avatar &guy, const itype_id &item_name, const int expected_moves )
{
    guy.remove_weapon();
    guy.clear_worn();
    item spawned_item( item_name, calendar::turn, 1 );
    item backpack( itype_backpack );
    REQUIRE( backpack.can_contain( spawned_item ).success() );
    auto item_iter = guy.worn.wear_item( guy, backpack, false, false );

    item_location backpack_loc( guy, & **item_iter );
    backpack_loc->put_in( spawned_item, pocket_type::CONTAINER );
    REQUIRE( backpack_loc->num_item_stacks() == 1 );
    item_location item_loc( backpack_loc, &backpack_loc->only_item() );
    CAPTURE( item_name );
    CAPTURE( item_loc->typeId() );

    guy.set_moves( 1000 );
    const int old_moves = guy.get_moves();
    REQUIRE( guy.wield( item_loc ) );
    CAPTURE( guy.get_wielded_item()->typeId() );
    int move_cost = old_moves - guy.get_moves();

    INFO( "Strength:" << guy.get_str() );
    CHECK( Approx( expected_moves ).epsilon( 0.1f ) == move_cost );
}

static void wield_check_from_ground( avatar &guy, const itype_id &item_name,
                                     const int expected_moves )
{
    item &spawned_item = get_map().add_item_or_charges( guy.pos_bub(), item( item_name, calendar::turn,
                         1 ) );
    item_location item_loc( map_cursor( guy.pos_abs() ), &spawned_item );
    CHECK( item_loc.obtain_cost( guy ) == Approx( expected_moves ).epsilon( 0.1f ) );
}

TEST_CASE( "Wield_test", "[wield]" )
{
    clear_map();
    item knife_hunting( itype_knife_hunting );
    item knife_combat( itype_knife_combat );
    item sheath( itype_sheath );

    REQUIRE( sheath.can_holster( knife_combat ) );

    map &here = get_map();

    Character &player = get_player_character();
    player.setpos( here, {player_pos.x(), player_pos.y(), 0} );
    player.set_body();
    player.clear_worn();
    player.remove_weapon();

    GIVEN( "A Player" ) {
        tripoint_bub_ms pos = player.pos_bub();

        GIVEN( "with an item is on the ground" ) {
            item &spawned_item = here.add_item_or_charges( pos, knife_hunting );
            item_location item_loc( map_cursor( pos ), &spawned_item );

            REQUIRE_FALSE( here.i_at( pos ).empty() );

            WHEN( "trying to wield it using its item_location" ) {
                REQUIRE( player.wield( item_loc, true ) );
                item_location wield_loc = player.get_wielded_item();

                THEN( "you wield the item" ) {
                    REQUIRE( wield_loc );
                    REQUIRE( wield_loc->typeId() == itype_knife_hunting );
                }

                AND_THEN( "the old item is removed" ) {
                    CHECK_FALSE( item_loc.get_item() );
                    REQUIRE( here.i_at( pos ).empty() );
                }
            }

            WHEN( "trying to wield it" ) {
                REQUIRE( player.wield( *item_loc ) );
                item_location wield_loc = player.get_wielded_item();

                THEN( "the old item is not removed" ) {
                    INFO( "should be made consistent with items from the inventory, but currently too many things rely on it working this way." );
                    CHECK( item_loc.get_item() );
                }

                AND_THEN( "you wield the item" ) {
                    REQUIRE( wield_loc );
                    REQUIRE( wield_loc->typeId() == itype_knife_hunting );
                }
            }
        }

        GIVEN( "you already wield an item" ) {
            player.remove_weapon();
            REQUIRE( player.wield( knife_hunting ) );

            item_location weapon = player.get_wielded_item();
            REQUIRE( weapon );

            INFO( "Would normally open dialogbox to unwield, returns false if test_mode" );
            WHEN( "you try to wield it again with item" ) {
                REQUIRE_FALSE( player.wield( *weapon ) );
            }

            AND_WHEN( "you try to wield it again with item_location" ) {
                REQUIRE_FALSE( player.wield( weapon ) );
            }

            AND_WHEN( "you try and wield another item" ) {
                REQUIRE_FALSE( player.wield( knife_combat ) );
            }
        }

        GIVEN( "Wielding a container with contained item" ) {
            REQUIRE( player.wield( sheath ) );
            item_location sheath_loc = player.get_wielded_item();
            item_location knife_loc = player.try_add( knife_combat );
            REQUIRE( sheath_loc.eventually_contains( knife_loc ) );

            WHEN( "trying to wield contained item" ) {
                REQUIRE_FALSE( player.wield( *knife_loc ) );
            }

            WHEN( "trying to wield contained item_location" ) {
                REQUIRE_FALSE( player.wield( knife_loc ) );
            }
        }
    }

    GIVEN( "A NPC" ) {
        point_bub_ms next_to = player_pos + point::north;
        npc &guy = spawn_npc( next_to, "test_talker" );

        g->load_npcs();
        clear_character( guy );

        GIVEN( "An item is on the ground" ) {
            item &spawned_item = here.add_item_or_charges( guy.pos_bub(), item( itype_knife_hunting,
                                 calendar::turn,
                                 1 ) );
            item_location item_loc( map_cursor( guy.pos_bub() ), &spawned_item );

            REQUIRE_FALSE( here.i_at( guy.pos_bub() ).empty() );

            WHEN( "trying to wield it using its item_location" ) {
                REQUIRE( guy.wield( item_loc, true ) );
                item_location weapon = guy.get_wielded_item();

                THEN( "they wield the item" ) {
                    CHECK( weapon );
                    CHECK( weapon->typeId() == itype_knife_hunting );
                }

                AND_THEN( "the old item is removed" ) {
                    CHECK_FALSE( item_loc );
                }
            }
        }

        GIVEN( "Wielding a container with contained item" ) {
            REQUIRE( guy.wield( sheath ) );
            item_location sheath_loc = guy.get_wielded_item();
            item_location knife_loc = guy.try_add( knife_combat );
            REQUIRE( sheath_loc.eventually_contains( knife_loc ) );

            WHEN( "trying to wield contained item" ) {
                REQUIRE_FALSE( guy.wield( *knife_loc ) );
            }

            WHEN( "trying to wield contained item_location" ) {
                REQUIRE_FALSE( guy.wield( knife_loc ) );
            }
        }

        AND_GIVEN( "you already wield an item" ) {
            // from the previous test

            guy.remove_weapon();
            REQUIRE( guy.wield( knife_hunting ) );

            item_location weapon = guy.get_wielded_item();
            REQUIRE( weapon );


            WHEN( "you try to wield it again with item" ) {
                INFO( "some tests require returning true, even if nothing changes" );
                REQUIRE( guy.wield( *weapon ) );
                THEN( "you wield the item again" ) {
                    REQUIRE( guy.get_wielded_item() );
                }
            }

            AND_WHEN( "you try to wield it again with item_location" ) {
                INFO( "should probably be made consisten with the item implementation" );
                REQUIRE_FALSE( guy.wield( weapon ) );
                // item_location new_weap = guy.get_wielded_item();
                // REQUIRE( new_weap );
                // THEN( "you un-wield the item" ) {
                //     REQUIRE( new_weap == weapon );
                // }
            }

            AND_WHEN( "you try and wield another item" ) {
                REQUIRE_FALSE( guy.can_stash( *weapon ) );
                REQUIRE( guy.wield( knife_combat ) );
                THEN( "you wield the other item" ) {
                    REQUIRE( guy.get_wielded_item() );
                    REQUIRE( guy.get_wielded_item()->typeId() == itype_knife_combat );
                }

                AND_THEN( "the old item gets dropped" ) {
                    REQUIRE_FALSE( here.i_at( guy.pos_bub() ).empty() );
                }
            }
        }
    }

}

TEST_CASE( "Wield_time_test", "[wield]" )
{
    clear_map();

    SECTION( "A knife in a sheath in cargo pants in a plastic bag in a backpack" ) {
        item backpack( itype_backpack );
        item plastic_bag( itype_bag_plastic );
        item cargo_pants( itype_pants_cargo );
        item sheath( itype_sheath );
        item knife( itype_knife_hunting );

        avatar guy;
        guy.set_body();
        auto item_iter = guy.worn.wear_item( guy, backpack, false, false );
        item_location backpack_loc( guy, & **item_iter );
        backpack_loc->put_in( plastic_bag, pocket_type::CONTAINER );
        REQUIRE( backpack_loc->num_item_stacks() == 1 );

        item_location plastic_bag_loc( backpack_loc, &backpack_loc->only_item() );
        plastic_bag_loc->put_in( cargo_pants, pocket_type::CONTAINER );
        REQUIRE( plastic_bag_loc->num_item_stacks() == 1 );

        item_location cargo_pants_loc( plastic_bag_loc, &plastic_bag_loc->only_item() );
        cargo_pants_loc->put_in( sheath, pocket_type::CONTAINER );
        REQUIRE( cargo_pants_loc->num_item_stacks() == 1 );

        item_location sheath_loc( cargo_pants_loc, &cargo_pants_loc->only_item() );
        sheath_loc->put_in( knife, pocket_type::CONTAINER );
        REQUIRE( sheath_loc->num_item_stacks() == 1 );

        item_location knife_loc( sheath_loc, &sheath_loc->only_item() );

        const int knife_obtain_cost = knife_loc.obtain_cost( guy );
        // This is kind of bad, on linux/OSX this value is 112.
        // On mingw-64 on wine, and probably on VS this is 111.
        // Most likely this is due to floating point differences, but I wasn't able to find where.
        CHECK( ( knife_obtain_cost == Approx( 105 ).margin( 1 ) ) );
    }

    SECTION( "Wielding without hand encumbrance" ) {
        avatar guy;
        clear_character( guy );

        wield_check_from_inv( guy, itype_aspirin, 300 );
        clear_character( guy );
        wield_check_from_inv( guy, itype_knife_combat, 325 );
        clear_character( guy );
        wield_check_from_ground( guy, itype_metal_tank, 300 );
        clear_character( guy );
    }
}
