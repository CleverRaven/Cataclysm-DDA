#include <climits>
#include <list>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "monster.h"
#include "mtype.h"
#include "player.h"
#include "point.h"
#include "string_id.h"
#include "type_id.h"

static player &get_sanitized_player( )
{
    avatar &player_character = get_avatar();
    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( player_character.takeoff( player_character.i_at( -2 ), &temp ) ) {}
    player_character.inv.clear();
    player_character.remove_weapon();

    return player_character;
}

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
    player &player_character = get_sanitized_player();

    g->clear_zombies();
    item &test_item = player_character.i_add( item( "bot_manhack", 0, item::default_charges_tag{} ) );

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

