#include "catch/catch.hpp"

#include <list>
#include <memory>

#include "avatar.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "iuse_actor.h"
#include "monster.h"
#include "mtype.h"
#include "pimpl.h"
#include "player.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static monster *find_adjacent_monster( const tripoint &pos )
{
    tripoint target = pos;
    for( target.x = pos.x - 1; target.x <= pos.x + 1; target.x++ ) {
        for( target.y = pos.y - 1; target.y <= pos.y + 1; target.y++ ) {
            if( target == pos ) {
                continue;
            }
            if( monster *const candidate = g->critter_at<monster>( target ) ) {
                return candidate;
            }
        }
    }
    return nullptr;
}

TEST_CASE( "manhack", "[iuse_actor][manhack]" )
{
    clear_avatar();
    player &player_character = get_avatar();

    g->clear_zombies();
    item &test_item = player_character.i_add( item( "bot_manhack", calendar::turn_zero,
                      item::default_charges_tag{} ) );

    REQUIRE( player_character.has_item( test_item ) );

    monster *new_manhack = find_adjacent_monster( player_character.pos() );
    REQUIRE( new_manhack == nullptr );

    player_character.invoke_item( &test_item );

    REQUIRE( !player_character.has_item_with( []( const item & it ) {
        return it.typeId() == itype_id( "bot_manhack" );
    } ) );

    new_manhack = find_adjacent_monster( player_character.pos() );
    REQUIRE( new_manhack != nullptr );
    REQUIRE( new_manhack->type->id == mtype_id( "mon_manhack" ) );
    g->clear_zombies();
}

TEST_CASE( "tool transform when activated", "[iuse][tool][transform]" )
{
    player &dummy = get_avatar();
    clear_avatar();

    GIVEN( "flashlight with a charged battery installed" ) {
        item flashlight( "flashlight" );
        item bat_cell( "light_battery_cell" );
        REQUIRE( flashlight.is_reloadable_with( itype_id( "light_battery_cell" ) ) );

        // Charge the battery
        const int bat_charges = bat_cell.ammo_capacity( ammotype( "battery" ) );
        bat_cell.ammo_set( bat_cell.ammo_default(), bat_charges );
        REQUIRE( bat_cell.ammo_remaining() == bat_charges );

        // Put battery in flashlight
        REQUIRE( flashlight.contents.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) );
        ret_val<bool> result = flashlight.put_in( bat_cell, item_pocket::pocket_type::MAGAZINE_WELL );
        REQUIRE( result.success() );
        REQUIRE( flashlight.magazine_current() );

        WHEN( "flashlight is turned on" ) {
            // Activation occurs via the iuse_transform actor
            const use_function *use = flashlight.type->get_use( "transform" );
            REQUIRE( use != nullptr );
            const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );
            actor->use( dummy, flashlight, false, dummy.pos() );

            THEN( "it becomes active" ) {
                CHECK( flashlight.active );
            }
            THEN( "the battery remains installed" ) {
                CHECK( flashlight.magazine_current() );
            }
            THEN( "name indicates (on) status" ) {
                CHECK( flashlight.tname() == "flashlight (on)" );
            }
        }
    }
}
