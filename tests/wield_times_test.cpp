#include "cata_catch.h"

#include <list>

#include "avatar.h"
#include "calendar.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_aspirin( "aspirin" );
static const itype_id itype_knife_combat( "knife_combat" );
static const itype_id itype_metal_tank( "metal_tank" );

static void wield_check_from_inv( avatar &guy, const itype_id &item_name, const int expected_moves )
{
    guy.remove_weapon();
    guy.clear_worn();
    item spawned_item( item_name, calendar::turn, 1 );
    item backpack( "backpack" );
    REQUIRE( backpack.can_contain( spawned_item ).success() );
    auto item_iter = guy.worn.wear_item( guy, backpack, false, false );
    REQUIRE( guy.mutation_value( "obtain_cost_multiplier" ) == 1.0 );

    item_location backpack_loc( guy, & **item_iter );
    backpack_loc->put_in( spawned_item, item_pocket::pocket_type::CONTAINER );
    REQUIRE( backpack_loc->num_item_stacks() == 1 );
    item_location item_loc( backpack_loc, &backpack_loc->only_item() );
    CAPTURE( item_name );
    CAPTURE( item_loc->typeId() );

    guy.set_moves( 1000 );
    const int old_moves = guy.moves;
    REQUIRE( guy.wield( item_loc ) );
    CAPTURE( guy.get_wielded_item()->typeId() );
    int move_cost = old_moves - guy.moves;

    INFO( "Strength:" << guy.get_str() );
    CHECK( Approx( expected_moves ).epsilon( 0.1f ) == move_cost );
}

static void wield_check_from_ground( avatar &guy, const itype_id &item_name,
                                     const int expected_moves )
{
    item &spawned_item = get_map().add_item_or_charges( guy.pos(), item( item_name, calendar::turn,
                         1 ) );
    item_location item_loc( map_cursor( guy.pos() ), &spawned_item );
    CHECK( item_loc.obtain_cost( guy ) == Approx( expected_moves ).epsilon( 0.1f ) );
}

TEST_CASE( "Wield_time_test", "[wield]" )
{
    clear_map();

    SECTION( "A knife in a sheath in cargo pants in a plastic bag in a backpack" ) {
        item backpack( "backpack" );
        item plastic_bag( "bag_plastic" );
        item cargo_pants( "pants_cargo" );
        item sheath( "sheath" );
        item knife( "knife_hunting" );

        avatar guy;
        guy.set_body();
        auto item_iter = guy.worn.wear_item( guy, backpack, false, false );
        item_location backpack_loc( guy, & **item_iter );
        backpack_loc->put_in( plastic_bag, item_pocket::pocket_type::CONTAINER );
        REQUIRE( backpack_loc->num_item_stacks() == 1 );
        REQUIRE( guy.mutation_value( "obtain_cost_multiplier" ) == 1.0 );

        item_location plastic_bag_loc( backpack_loc, &backpack_loc->only_item() );
        plastic_bag_loc->put_in( cargo_pants, item_pocket::pocket_type::CONTAINER );
        REQUIRE( plastic_bag_loc->num_item_stacks() == 1 );

        item_location cargo_pants_loc( plastic_bag_loc, &plastic_bag_loc->only_item() );
        cargo_pants_loc->put_in( sheath, item_pocket::pocket_type::CONTAINER );
        REQUIRE( cargo_pants_loc->num_item_stacks() == 1 );

        item_location sheath_loc( cargo_pants_loc, &cargo_pants_loc->only_item() );
        sheath_loc->put_in( knife, item_pocket::pocket_type::CONTAINER );
        REQUIRE( sheath_loc->num_item_stacks() == 1 );

        item_location knife_loc( sheath_loc, &sheath_loc->only_item() );

        const int knife_obtain_cost = knife_loc.obtain_cost( guy );
        // This is kind of bad, on linux/OSX this value is 112.
        // On mingw-64 on wine, and probably on VS this is 111.
        // Most likely this is due to floating point differences, but I wasn't able to find where.
        CHECK( ( knife_obtain_cost == 112 || knife_obtain_cost == 111 ) );
    }

    SECTION( "Wielding without hand encumbrance" ) {
        avatar guy;
        clear_character( guy );
        REQUIRE( guy.mutation_value( "obtain_cost_multiplier" ) == 1.0 );

        wield_check_from_inv( guy, itype_aspirin, 300 );
        clear_character( guy );
        wield_check_from_inv( guy, itype_knife_combat, 325 );
        clear_character( guy );
        wield_check_from_ground( guy, itype_metal_tank, 300 );
        clear_character( guy );
    }
}
