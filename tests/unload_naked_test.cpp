#include <list>
#include <memory>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "optional.h"
#include "player_helpers.h"
#include "type_id.h"

/**
 * When a player has no open inventory to place unloaded bullets, but there is room in the gun being
 * unloaded the bullets are dropped.
 * issue# 29839
 */

TEST_CASE( "unload_revolver_naked_one_bullet", "[unload][nonmagzine]" )
{
    clear_avatar();
    clear_map();

    Character &dummy = get_player_character();
    avatar &player_character = get_avatar();

    // revolver with only one of six bullets
    item revolver( "ruger_redhawk" );
    revolver.ammo_set( revolver.ammo_default(), 1 );

    // wield the revolver
    REQUIRE( !player_character.is_armed( ) );
    REQUIRE( player_character.wield( revolver ) );
    REQUIRE( player_character.is_armed( ) );

    CHECK( player_character.weapon.ammo_remaining() == 1 );

    // Unload weapon
    item_location revo_loc( dummy, &player_character.weapon );
    player_character.moves = 100;
    REQUIRE( player_character.unload( revo_loc ) );
    player_character.activity.do_turn( player_character );

    // No bullets in wielded gun
    CHECK( player_character.weapon.ammo_remaining() == 0 );

    // No bullets in inventory
    const std::vector<item *> bullets = dummy.items_with( []( const item & item ) {
        return item.is_ammo();
    } );
    CHECK( bullets.empty() );
}

TEST_CASE( "unload_revolver_naked_fully_loaded", "[unload][nonmagzine]" )
{
    clear_avatar();
    clear_map();

    Character &dummy = get_player_character();
    avatar &player_character = get_avatar();

    // revolver fully loaded
    item revolver( "ruger_redhawk" );
    revolver.ammo_set( revolver.ammo_default(), revolver.remaining_ammo_capacity() );

    // wield the revolver
    REQUIRE( !player_character.is_armed( ) );
    REQUIRE( player_character.wield( revolver ) );
    REQUIRE( player_character.is_armed( ) );

    CHECK( player_character.weapon.remaining_ammo_capacity() == 0 );

    // Unload weapon
    item_location revo_loc( dummy, &player_character.weapon );
    player_character.moves = 100;
    REQUIRE( player_character.unload( revo_loc ) );
    player_character.activity.do_turn( player_character );

    // No bullets in wielded gun
    CHECK( player_character.weapon.ammo_remaining() == 0 );

    // No bullets in inventory
    const std::vector<item *> bullets = dummy.items_with( []( const item & item ) {
        return item.is_ammo();
    } );
    CHECK( bullets.empty() );
}
