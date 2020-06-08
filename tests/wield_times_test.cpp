#include <cstdio>
#include <string>
#include <list>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player.h"
#include "player_helpers.h"
#include "item.h"
#include "point.h"

static void wield_check_from_inv( avatar &guy, const itype_id &item_name, const int expected_moves )
{
    guy.remove_weapon();
    guy.worn.clear();
    item spawned_item( item_name, calendar::turn, 1 );
    item backpack( "backpack" );
    REQUIRE( backpack.can_contain( spawned_item ) );
    guy.worn.push_back( backpack );

    item_location backpack_loc( guy, &guy.worn.back() );
    backpack_loc->put_in( spawned_item, item_pocket::pocket_type::CONTAINER );
    REQUIRE( backpack_loc->contents.num_item_stacks() == 1 );
    item_location item_loc( backpack_loc, &backpack_loc->contents.only_item() );
    CAPTURE( item_name );
    CAPTURE( item_loc->typeId() );

    guy.set_moves( 1000 );
    const int old_moves = guy.moves;
    REQUIRE( guy.wield( item_loc ) );
    CAPTURE( guy.weapon.typeId() );
    int move_cost = old_moves - guy.moves;

    INFO( "Strength:" << guy.get_str() );
    CHECK( Approx( expected_moves ).epsilon( 0.1f ) == move_cost );
}

static void wield_check_from_ground( avatar &guy, const itype_id &item_name,
                                     const int expected_moves )
{
    item &spawned_item = g->m.add_item_or_charges( guy.pos(), item( item_name, calendar::turn, 1 ) );
    item_location item_loc( map_cursor( guy.pos() ), &spawned_item );
    CHECK( item_loc.obtain_cost( guy ) == Approx( expected_moves ).epsilon( 0.1f ) );
}

TEST_CASE( "Wield time test", "[wield]" )
{
    clear_map();

    SECTION( "A knife in a sheath in cargo pants in a plastic bag in a backpack" ) {
        item backpack( "backpack" );
        item plastic_bag( "bag_plastic" );
        item cargo_pants( "pants_cargo" );
        item sheath( "sheath" );
        item knife( "knife_hunting" );

        avatar guy;
        guy.worn.push_back( backpack );
        item_location backpack_loc( guy, &guy.worn.back() );
        backpack_loc->put_in( plastic_bag, item_pocket::pocket_type::CONTAINER );
        REQUIRE( backpack_loc->contents.num_item_stacks() == 1 );

        item_location plastic_bag_loc( backpack_loc, &backpack_loc->contents.only_item() );
        plastic_bag_loc->put_in( cargo_pants, item_pocket::pocket_type::CONTAINER );
        REQUIRE( plastic_bag_loc->contents.num_item_stacks() == 1 );

        item_location cargo_pants_loc( plastic_bag_loc, &plastic_bag_loc->contents.only_item() );
        cargo_pants_loc->put_in( sheath, item_pocket::pocket_type::CONTAINER );
        REQUIRE( cargo_pants_loc->contents.num_item_stacks() == 1 );

        item_location sheath_loc( cargo_pants_loc, &cargo_pants_loc->contents.only_item() );
        sheath_loc->put_in( knife, item_pocket::pocket_type::CONTAINER );
        REQUIRE( sheath_loc->contents.num_item_stacks() == 1 );

        item_location knife_loc( sheath_loc, &sheath_loc->contents.only_item() );

        const int knife_obtain_cost = knife_loc.obtain_cost( guy );
        REQUIRE( knife_obtain_cost == 1257 );
    }

    SECTION( "Wielding without hand encumbrance" ) {
        avatar guy;
        clear_character( guy );

        wield_check_from_inv( guy, itype_id( "halberd" ), 612 );
        clear_character( guy );
        wield_check_from_inv( guy, itype_id( "aspirin" ), 375 );
        clear_character( guy );
        wield_check_from_inv( guy, itype_id( "knife_combat" ), 412 );
        clear_character( guy );
        wield_check_from_ground( guy, itype_id( "metal_tank" ), 300 );
        clear_character( guy );
    }
}
